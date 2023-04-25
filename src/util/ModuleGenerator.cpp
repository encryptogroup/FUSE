/*
 * MIT License
 *
 * Copyright (c) 2022 Nora Khayata
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED
 * "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "ModuleGenerator.h"

#include <random>

#include "BristolFrontend.h"

namespace fuse::util {

core::CircuitContext
generateCircuitWithNumberOfNodes(unsigned long totalNumberOfNodes) {
  // calculate fractions of different type of nodes
  long numInputs = (totalNumberOfNodes / 10) + 1;
  long numConstants = (totalNumberOfNodes / 10) + 1;
  long numCustomOps = (totalNumberOfNodes / 1000) + 1;
  long numCalls = (totalNumberOfNodes / 10) + 1;
  long numOutputs = (totalNumberOfNodes / 10) + 1;
  long numOperations =
      totalNumberOfNodes -
      (numInputs + numOutputs + numConstants + numCustomOps + numCalls);

  // generate circuit with specified node types
  frontend::CircuitBuilder cb("circuit_with_" +
                              std::to_string(totalNumberOfNodes) + "_nodes");
  size_t secureInt = cb.addDataType(core::ir::PrimitiveType::Int32);
  size_t plaintextInt = cb.addDataType(core::ir::PrimitiveType::Int32,
                                       core::ir::SecurityLevel::Plaintext);
  for (int i = 0; i < numInputs; ++i) {
    cb.addInputNode({secureInt});
  }
  for (int i = 0; i < numConstants; ++i) {
    cb.addConstantNodeWithPayload(i);
  }
  unsigned long currId;
  for (long i = 0; i < numOperations; ++i) {
    unsigned long in = static_cast<unsigned long>(i);
    currId = cb.addNode(core::ir::PrimitiveOperation::Mul, {in, in + 1});
  }

  srand(time(NULL));
  std::random_device rd;
  std::mt19937_64 e(rd());
  std::uniform_int_distribution<unsigned long> dist(0, currId);
  auto gen = std::bind(dist, e);

  // the nodes generated below are to be always optimizable by a dead node
  // optimization
  for (int i = 0; i < numCustomOps; ++i) {
    unsigned int in = static_cast<unsigned int>(i);
    cb.addNodeWithCustomOperation("custom_op_" + std::to_string(i), {},
                                  {in, in + 1}, {});
  }
  for (int i = 0; i < numCalls; ++i) {
    unsigned long in = static_cast<unsigned long>(i);
    cb.addCallToSubcircuitNode({in, in + 1}, "subcircuit_" + std::to_string(i));
  }

  // outputs only use one of the non-custom and non-call nodes as input
  for (int i = 0; i < numOutputs; ++i) {
    cb.addOutputNode({plaintextInt}, {gen()});
  }

  // finish circuit building and return context
  cb.finish();
  core::CircuitContext res(cb);
  return res;
}

core::ModuleContext generateModuleWithCall() {
  frontend::ModuleBuilder mb;
  auto c1 = mb.addCircuit("c1");
  auto c2 = mb.addCircuit("c2");
  auto secBool1 = c1->addDataType(core::ir::PrimitiveType::Bool);
  auto plainBool1 = c1->addDataType(core::ir::PrimitiveType::Bool,
                                    core::ir::SecurityLevel::Plaintext);
  auto in11 = c1->addInputNode({secBool1}, "party:1");
  auto in12 = c1->addInputNode({secBool1}, "party:2");

  auto secBool2 = c2->addDataType(core::ir::PrimitiveType::Bool);
  auto plainBool2 = c2->addDataType(core::ir::PrimitiveType::Bool,
                                    core::ir::SecurityLevel::Plaintext);
  auto in21 = c2->addInputNode({secBool2}, "party:1");
  auto in22 = c2->addInputNode({secBool2}, "party:2");
  auto and2 = c2->addNode(core::ir::PrimitiveOperation::And, {in21, in22});
  c2->addOutputNode({plainBool2}, {and2});

  auto call = c1->addCallToSubcircuitNode({in11, in12}, "c2");
  c1->addOutputNode({plainBool1}, {call});
  mb.setEntryCircuitName("c1");
  mb.finish();

  core::ModuleContext context(mb);
  return context;
}

core::ModuleContext generateModuleWithSha512Calls(unsigned numberOfShaCalls) {
  assert(numberOfShaCalls > 0);
  // directory: FUSE/examples/bristol_circuits/sha_512.bristol -> name is going
  // to be sha_512
  const std::string pathToSha512 =
      "../../examples/bristol_circuits/sha_512.bristol";
  // SHA512: (1024, 512) -> 512
  // maps input buffer and input chaining state to the next state
  // 1024 bits -> 128 bytes
  auto shaCircuit = fuse::frontend::loadFUSEFromBristol(pathToSha512);
  // create entry circuit with calls to sha circuit
  frontend::ModuleBuilder mb;
  mb.addSerializedCircuit(shaCircuit.getBufferPointer(),
                          shaCircuit.getBufferSize());
  auto circ = mb.addCircuit("main");
  mb.setEntryCircuitName("main");

  auto secureBooleanType = circ->addDataType(core::ir::PrimitiveType::Bool);
  auto plaintextBooleanType = circ->addDataType(
      core::ir::PrimitiveType::Bool, core::ir::SecurityLevel::Plaintext);

  // contains the inputBuffer nodes
  std::vector<uint64_t> inputBuffer;
  // contains the chainingState nodes
  std::vector<uint64_t> chainingState;

  for (int i = 0; i < 1024; ++i) {
    inputBuffer.push_back(circ->addInputNode(secureBooleanType, "party:1"));
  }

  for (int i = 0; i < 512; ++i) {
    chainingState.push_back(circ->addInputNode(secureBooleanType, "party:2"));
  }

  std::vector<uint32_t> offsets;
  uint64_t currentCall;
  for (int i = 0; i < numberOfShaCalls; ++i) {
    // construct input node identifiers: input buffer and current chaining state
    std::vector<uint64_t> inputs;
    inputs.reserve(inputBuffer.size() + chainingState.size());
    inputs.insert(inputs.end(), inputBuffer.begin(), inputBuffer.end());
    inputs.insert(inputs.end(), chainingState.begin(), chainingState.end());
    // add call node for the next chaining state
    currentCall = circ->addCallToSubcircuitNode(inputs, offsets, "sha_512");
    // after first call: introduce offsets for the next calls
    if (offsets.empty()) {
      for (int i = 0; i < 1024; ++i) {
        offsets.push_back(0);
      }
      for (int i = 0; i < 512; ++i) {
        offsets.push_back(i);
      }
    }
    // now add the new call node as the input chaining state for the next call
    chainingState.clear();
    for (int i = 0; i < 512; ++i) {
      chainingState.push_back(currentCall);
    }
  }

  // after adding the calls, define the output nodes as the current chaining
  // state
  for (unsigned i = 0; i < 512; ++i) {
    const std::vector<uint64_t> inputVec = {currentCall};
    const std::vector<uint32_t> offsetVec = {i};
    circ->addOutputNode(plaintextBooleanType, inputVec, offsetVec);
  }

  core::ModuleContext context(mb);
  return context;
}

} // namespace fuse::util
