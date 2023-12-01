#ifndef FUSE_BUILDMNIST_HPP
#define FUSE_BUILDMNIST_HPP
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
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include "IR.h"
#include "NodeSuccessorsAnalysis.h"
#include "BristolFrontend.h"
#include "DOTBackend.h"

std::unique_ptr<fuse::core::ir::DataTypeTableT> copyType (fuse::core::ir::DataTypeTableT* ptr) {
    auto cp = std::make_unique<fuse::core::ir::DataTypeTableT>();
    cp->security_level = ptr->security_level;
    cp->primitive_type = ptr->primitive_type;
    cp->shape = ptr->shape;
    cp->data_type_annotations = ptr->data_type_annotations;
    return std::move(cp);
}



void generateMain(fuse::frontend::CircuitBuilder* circ) {
    size_t dt = circ->addDataType(fuse::core::ir::PrimitiveType::Int32, fuse::core::ir::SecurityLevel::Secure);
    // create input values:
    // party 1 : vector of 784 elements (the 28x28 matrix inlined)
    // party 2 : the different weights of the ML models (also done in Silph), these are:
    //              - for FC1: a 784x128 weight matrix, stored column-wise (each column holds 128 elements; for more locality in dotproducts)
    //              - for FC2: a 128x128 weight matrix, stored column-wise
    //              - for FC3: a 128x10 weight matrix, stored column-wise
    //              ===> everything inlined, party 2 holds 100352 +16384 + 1280 = 118016 input values
    constexpr int inputsPartyOne = 28*28;
    constexpr int inputsPartyTwo = 784*128 + 128*128 + 128*10;
    std::array<uint64_t, inputsPartyOne> inputOneIds;
    std::array<uint64_t, inputsPartyTwo> inputTwoIds;
    for (int i = 0; i < inputsPartyOne; ++i) {
        inputOneIds[i] = circ->addInputNode(dt,"1");
    }
    for (int i = 0; i < inputsPartyTwo; ++i) {
        inputTwoIds[i] =  circ->addInputNode(dt,"party:2");
    }

    // Add first Fully Connected (FC) layer: 1x784 | 784x128 ==> 1x128
    //          - Input size: 28x28 (1x784), matrix multiplication with (784x128) matrix Output: 128 (1x128)
    std::array<uint64_t, 128> outputFC1;
    constexpr int colSize1 = 784;
    for (int i = 0; i < outputFC1.size(); ++i) {
        // call the dotprod-784 circuit for computing each value in the output vector
        std::vector<uint64_t> callInputs (inputOneIds.begin(), inputOneIds.end());
        // copy one row at a time from the weight matrix
        auto beginIterator = inputTwoIds.begin() + (i * colSize1);
        auto endIterator = inputTwoIds.begin() + ( (i+1) * colSize1);
        std::copy(beginIterator, endIterator, std::back_inserter(callInputs));
        // Identifier CircuitBuilder::addCallToSubcircuitNode(const std::vector<Identifier> &input_identifiers, const std::string &subcircuit_name, const std::string &node_annotations)
        outputFC1[i] = circ->addCallToSubcircuitNode(callInputs, "dotprod784");
    }

    // Add ReLU layer: 1x128 ==> 1x128
    //          we only have ReLU(x) for a single 32-bit integer value -> add 128 call nodes
    std::array<uint64_t, 128> outputReLU1;
    for (int i = 0; i < outputReLU1.size(); ++i) {
        std::vector<uint64_t> callInput {outputFC1.at(i)};
        outputReLU1[i] = circ->addCallToSubcircuitNode(callInput, "relu");
    }

    // Add next FC layer: 1x128 | 128x128 ==> 1x128
    //          the 128x128 is the second part of party 2's input
    std::array<uint64_t, 128> outputFC2;
    constexpr int colSize2 = 128;
    constexpr int inputOffset = 784*128; // this is the first index of inputTwoIds that holds the weight matrix for this layer
    for (int i = 0; i < outputFC2.size(); ++i) {
        // call dotprod for each element of the new matrix
        std::vector<uint64_t> callInputs (outputReLU1.begin(), outputReLU1.end());
        // copy one row at a time from the weight matrix
        auto beginIterator = inputTwoIds.begin() + inputOffset + (i * colSize2);
        auto endIterator = inputTwoIds.begin() + inputOffset + ( (i+1) * colSize2);
        std::copy(beginIterator, endIterator, std::back_inserter(callInputs));
        // Identifier CircuitBuilder::addCallToSubcircuitNode(const std::vector<Identifier> &input_identifiers, const std::string &subcircuit_name, const std::string &node_annotations)
        outputFC2[i] = circ->addCallToSubcircuitNode(callInputs, "dotprod128");
    }

    // Add next ReLU layer after FC2: 1x128 ==> 1x128
    //          again, we only have ReLU(x) for a single 32-bit integer value -> add 128 call nodes
    std::array<uint64_t, 128> outputReLU2;
    for (int i = 0; i < outputReLU2.size(); ++i) {
        std::vector<uint64_t> callInput {outputFC2.at(i)};
        outputReLU2[i] = circ->addCallToSubcircuitNode(callInput, "relu");
    }

    // Add next FC layer: 1x128 | 128x10 ==> 1x10
    //          the 128x10 is the second part of party 2's input
    std::array<uint64_t, 10> outputFC3;
    constexpr int colSize3 = 128;
    constexpr int inputOffset3 = 784*128 + 128*128; // this is the first index of inputTwoIds that holds the weight matrix for this layer
    for (int i = 0; i < outputFC3.size(); ++i) {
        // call dotprod for each element of the new matrix
        std::vector<uint64_t> callInputs (outputReLU2.begin(), outputReLU2.end());
        // copy one row at a time from the weight matrix
        auto beginIterator = inputTwoIds.begin() + inputOffset3 + (i * colSize3);
        auto endIterator = inputTwoIds.begin() + inputOffset3 + ( (i+1) * colSize3);
        std::copy(beginIterator, endIterator, std::back_inserter(callInputs));
        // Identifier CircuitBuilder::addCallToSubcircuitNode(const std::vector<Identifier> &input_identifiers, const std::string &subcircuit_name, const std::string &node_annotations)
        outputFC2[i] = circ->addCallToSubcircuitNode(callInputs, "dotprod128");
    }

    // Add next ReLU layer after FC3: 1x10 ==> 1x10
    //          again, we only have ReLU(x) for a single 32-bit integer value -> add 10 call nodes
    std::array<uint64_t, 10> outputReLU3;
    for (int i = 0; i < outputReLU3.size(); ++i) {
        std::vector<uint64_t> callInput {outputFC3.at(i)};
        outputReLU3[i] = circ->addCallToSubcircuitNode(callInput, "relu");
        // add output nodes for the results of the last ReLU layer ==> 10 output nodes of int32 values
        circ->addOutputNode(dt, {outputReLU3[i]});
    }
}

// dot product circuit that takes two vectors of size Vecsize with int32 elements as input
template<std::size_t Vecsize>
void generateDotProduct(fuse::frontend::CircuitBuilder* circ) {
    // add input nodes for both input vectors which have elements of type Int32
    size_t dt = circ->addDataType(fuse::core::ir::PrimitiveType::Int32, fuse::core::ir::SecurityLevel::Secure);
    std::array<uint64_t, Vecsize> inVec1;
    std::array<uint64_t, Vecsize> inVec2;
    for(int i = 0; i < Vecsize; ++i) {
        inVec1[i] = circ->addInputNode(dt,"");
    }
    for(int i = 0; i < Vecsize; ++i) {
        inVec2[i] = circ->addInputNode(dt,"");
    }
    // add element-wise multiplication layer
    std::array<uint64_t, Vecsize> mulNodes;
    for (int i = 0; i < Vecsize; ++i) {
        mulNodes[i] = circ->addNode(fuse::core::ir::PrimitiveOperation::Mul, {inVec1[i], inVec2[i]});
    }

    //  build up sum-tree that always adds the current nodes i and i+1, i+2 and i+3 and so on for each layer
    std::vector<uint64_t> currLayer;
    std::copy(mulNodes.begin(), mulNodes.end(), std::back_inserter(currLayer));
    std::vector<uint64_t> nextLayer;
    while (currLayer.size() != 1) {
        for (int i = 0; i < currLayer.size(); i = i+2) {
            if (i+1 < currLayer.size()) {
                nextLayer.push_back(
                    circ->addNode(fuse::core::ir::PrimitiveOperation::Add, {currLayer.at(i), currLayer.at(i+1)})
                );
            } else { // i+1 out of bounds -> nothing to connect to, keep for next layer
                nextLayer.push_back(currLayer.at(i));
            }
        }
        currLayer = nextLayer;
        nextLayer.clear();
    }
    circ->addOutputNode(dt, {currLayer.at(0)});
    // inefficient fallback solution if the other approach with log(Vecsize) addition layers doesn't work:
        // uint64_t currSum = circ->addNode(fuse::core::ir::PrimitiveOperation::Add, {mulNodes[0], mulNodes[1]});
        // for (int i = 2; i < Vecsize; ++i) {
        //     currSum = circ->addNode(fuse::core::ir::PrimitiveOperation::Add, {prevSum, mulNodes[i]});
        // }
        // // set output node to last sum node
        // circ->addOutputNode(dt, {currSum});
}

// ReLU circuit for a single 32-bit value
fuse::core::CircuitContext generateReLU() {
    // build ReLU circuit from comparison: load depth-optimized comparison (x > y) from bristol
    const std::string gt32Path = "../../examples/bristol_circuits/int_gt32_depth.bristol";
    auto context = fuse::frontend::loadFUSEFromBristol(gt32Path);
    auto relu = context.getMutableCircuitWrapper();
    auto reluPtr = relu._get(); // pointer to underlying unpacked c++ object, things needed to get hacky at some point
    reluPtr->name = "relu";

    // -------------- declare new bool and int Types --------------
    auto boolType = std::make_unique<fuse::core::ir::DataTypeTableT>();
    boolType->primitive_type = fuse::core::ir::PrimitiveType::Bool;
    boolType->security_level = fuse::core::ir::SecurityLevel::Secure;
    auto int32Type = std::make_unique<fuse::core::ir::DataTypeTableT>();
    int32Type->primitive_type = fuse::core::ir::PrimitiveType::Int32;
    int32Type->security_level = fuse::core::ir::SecurityLevel::Secure;

    // -------------- 1) Input: take 32-bit as first input --------------
    std::unordered_set<uint64_t> inputIDsToRemove;
    std::array<uint64_t, 32> firstOldInput;
    std::array<uint64_t, 32> secondOldInput;
    // store which input flows to which nodes in the circuit, these need to be updated
    // after the split operation on the input (A2B conversion)
    std::unordered_map<uint64_t, std::unordered_set<uint64_t>> nodeSuccessors = fuse::passes::getNodeSuccessors(relu);
    // get first 32 input bits
    for (int i = 0; i < 32; ++i) {
        // first input: replace by int32 input value and remove old input nodes
        auto oldInput1 = relu.getInputNodeIDs()[i];
        firstOldInput[i] = oldInput1;
        inputIDsToRemove.insert(oldInput1);
        // second input: replace with constant for comparison
        auto oldInput2 = relu.getInputNodeIDs()[i+32];
        secondOldInput[i] = oldInput2;
    }

    // second set of old inputs: change to constant nodes for the special comparison for ReLU!
    int numOfReplacedConstants = 0;
    for (auto toReplace : secondOldInput) {
        auto nodePtr = relu.getNodeWithID(toReplace)._get();
        nodePtr->operation = fuse::core::ir::PrimitiveOperation::Constant;
        nodePtr->output_datatypes.push_back(copyType(boolType.get()));
        flexbuffers::Builder fbb;
        // first 31 wires [0-30] should be set to one
        if (numOfReplacedConstants < 31) {
            fbb.Int(1);
        } else { // here, numOfReplaceConstants = 31 -> set to zero
            fbb.Int(0);
        }
        fbb.Finish();
        // last wire should be set to zero --> we get the number 11111..0
        nodePtr->payload = fbb.GetBuffer();
        ++numOfReplacedConstants;
    }

    // remove first set of old input nodes and corresponding input datatypes
    relu.removeNodes(inputIDsToRemove);
    reluPtr->input_datatypes.clear();
    reluPtr->input_datatypes.push_back(copyType(int32Type.get()));

    // add new single input with uint32 as input type
    auto newInput = relu.addNode(0);
    newInput.setPrimitiveOperation(fuse::core::ir::PrimitiveOperation::Input);
    newInput._get()->input_datatypes.push_back(copyType(int32Type.get()));

    // -------------- add Split node and rewire the new 32-bit input for the comparison --------------
    std::array<uint64_t, 1> splitIn {newInput.getNodeID()};
    auto split = relu.addNode(1, fuse::core::ir::PrimitiveOperation::Split, splitIn);
    split._get()->input_datatypes.push_back(copyType(int32Type.get()));
    split._get()->output_datatypes.push_back(copyType(boolType.get()));
    split._get()->num_of_outputs = 32; // 32 boolean values as outputs because we operate over 32-bit integers
    // update successors of old input values with new input and offsets
    short offset = 0;
    for (auto oldID : firstOldInput) {
        auto successors = nodeSuccessors.at(oldID);
        for (auto dependentNodeID : successors) {
            auto depNode = relu.getNodeWithID(dependentNodeID);
            depNode._get()->input_offsets.resize(depNode.getNumberOfInputs());
            for (short i = 0; i < depNode.getNumberOfInputs(); ++i) {
                if (depNode._get()->input_identifiers.at(i) == oldID) {
                    depNode._get()->input_identifiers.at(i) = split.getNodeID();
                    depNode._get()->input_offsets.at(i) = offset;
                    ++offset;
                } else {
                    depNode._get()->input_offsets.at(i) = 0;
                }
            }
        }
    }

    // -------------- 2) Merge comparison output into 32-bit integer via Merge node --------------
    std::unordered_set<uint64_t> compOutputSet; // output nodes to delete later
    std::vector<uint64_t> compOutputVec; // nodes that gave the output value, so the inputs of the output nodes
    for (uint64_t compOutID : relu.getOutputNodeIDs()) {
        compOutputSet.insert(compOutID); // old output node --> delete later
        auto compOutNode = relu.getNodeWithID(compOutID);
        compOutputVec.push_back(compOutNode.getInputNodeIDs()[0]); // input from output node --> store for rewiring
    }

    // -------------- add Merge node and rewire the new 32-bit input for the comparison --------------
    auto merge = relu.addNode();
    auto mPtr = merge._get();
    mPtr->operation = fuse::core::ir::PrimitiveOperation::Merge;
    mPtr->input_datatypes.push_back(copyType(boolType.get()));
    mPtr->output_datatypes.push_back(copyType(int32Type.get()));
    std::copy(compOutputVec.begin(), compOutputVec.end(), std::back_inserter(mPtr->input_identifiers));

    // -------------- add rest of computation from comparison -> relu --------------
    // relu(x) = (1 - Merge(cmp)) * inputVal
    // 1) constant one
    auto onePtr = relu.addNode()._get();
    onePtr->operation = fuse::core::ir::PrimitiveOperation::Constant;
    flexbuffers::Builder fbb;
    fbb.Int(1);
    fbb.Finish();
    onePtr->payload = fbb.GetBuffer();
    onePtr->output_datatypes.push_back(copyType(int32Type.get()));
    // 2) subtraction: 1 - merge
    std::array<uint64_t, 2> subInputs {onePtr->id, merge.getNodeID()};
    auto sub = relu.addNode();
    sub.setPrimitiveOperation(fuse::core::ir::PrimitiveOperation::Sub);
    sub.setInputNodeIDs(subInputs);
    // 3) multiplication: sub * input
    std::array<uint64_t, 2> mulInputs {sub.getNodeID(), newInput.getNodeID()};
    auto mul = relu.addNode();
    mul.setPrimitiveOperation(fuse::core::ir::PrimitiveOperation::Sub);
    mul.setInputNodeIDs(subInputs);

    // -------------- remove old output nodes, add single int32 output using the Mul's output value --------------
    relu.removeNodes(compOutputSet); // remove old output nodes
    // add new single node for output
    auto out = relu.addNode();
    out.setPrimitiveOperation(fuse::core::ir::PrimitiveOperation::Output);
    out.setInputNodeID(mul.getNodeID());
    // clear old output IDs and datatypes from circuit
    reluPtr->outputs.clear();
    reluPtr->output_datatypes.clear();
    reluPtr->output_datatypes.push_back(copyType(int32Type.get()));
    reluPtr->outputs.push_back(out.getNodeID());

    context.packCircuit();
    return context;
}

void generateArithmeticReLU(fuse::frontend::CircuitBuilder* circ) {
    auto dt = circ->addDataType(fuse::core::ir::PrimitiveType::Int32, fuse::core::ir::SecurityLevel::Secure);
    constexpr int compThreshold = INT32_MAX;
    auto inputNode = circ->addInputNode(dt);
    auto thresholdNode = circ->addConstantNodeWithPayload(compThreshold);
    auto oneNode = circ->addConstantNodeWithPayload((int32_t) 1);
    auto compNode = circ->addNode(fuse::core::ir::PrimitiveOperation::Gt, {inputNode, thresholdNode});
    auto subNode = circ->addNode(fuse::core::ir::PrimitiveOperation::Sub, {oneNode, compNode});
    auto mulNode = circ->addNode(fuse::core::ir::PrimitiveOperation::Mul, {subNode, inputNode});
    [[maybe_unused]]
    auto out = circ->addOutputNode({dt}, {mulNode});
}

fuse::frontend::ModuleBuilder generateSecureMLNN(){
    fuse::frontend::ModuleBuilder mod;

    // relu circuit for a single 32-bit integer value, similar to HyCC, Silph
    fuse::core::CircuitContext reluContext = generateReLU();
    mod.addSerializedCircuit(reluContext.getBufferPointer(), reluContext.getBufferSize());

    // dotproduct circuit on two 128-element vectors
    fuse::frontend::CircuitBuilder* dotprod128 = mod.addCircuit("dotprod128");
    generateDotProduct<128>(dotprod128);

    // dotproduct circuit on two 784-element vectors, used for the first input FC-layer
    fuse::frontend::CircuitBuilder* dotprod784 = mod.addCircuit("dotprod784");
    generateDotProduct<784>(dotprod784);

    // top-level main circuit, which contains the inlined code for the layers
    // and uses subcircuits for relu and dotproducts
    fuse::frontend::CircuitBuilder* main = mod.addCircuit("main");
    generateMain(main);

    mod.setEntryCircuitName("main");
    mod.finish();
    return mod;
}

fuse::frontend::ModuleBuilder generateArithmeticSecureMLNN() {
        fuse::frontend::ModuleBuilder mod;

    // relu circuit for a single 32-bit integer value, arithmetic-only version
    fuse::frontend::CircuitBuilder* relu = mod.addCircuit("relu");
    generateArithmeticReLU(relu);

    // dotproduct circuit on two 128-element vectors
    fuse::frontend::CircuitBuilder* dotprod128 = mod.addCircuit("dotprod128");
    generateDotProduct<128>(dotprod128);

    // dotproduct circuit on two 784-element vectors, used for the first input FC-layer
    fuse::frontend::CircuitBuilder* dotprod784 = mod.addCircuit("dotprod784");
    generateDotProduct<784>(dotprod784);

    // top-level main circuit, which contains the inlined code for the layers
    // and uses subcircuits for relu and dotproducts
    fuse::frontend::CircuitBuilder* main = mod.addCircuit("main");
    generateMain(main);

    mod.setEntryCircuitName("main");
    mod.finish();
    return mod;
}


#endif /* FUSE_BUILDMNIST_HPP */
