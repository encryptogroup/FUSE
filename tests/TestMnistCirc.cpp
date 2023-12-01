/*
 * MIT License
 *
 * Copyright (c) 2023 Nora Khayata
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
#include <gtest/gtest.h>

#include <bitset>
#include <filesystem>
#include <fstream>

#include "BristolFrontend.h"
#include "CallStackAnalysis.h"
#include "DOTBackend.h"
#include "Evaluator.hpp"
#include "IR.h"
#include "PlaintextInterpreter.hpp"
#include "build_mnist.hpp"

namespace fuse::tests::passes {

    //---------------------------------- CAUTION: these two functions were already used in TestInterpreter.cpp but lie in another namespace ----------------------------------

    std::unordered_map<uint64_t, bool> prepareEnvironment(const core::CircuitReadOnly *circ, std::string numberToRepresent) {
        auto in = circ->getNumberOfInputs();
        auto size = numberToRepresent.size();
        assert(in == size);
        std::unordered_map<uint64_t, bool> result;
        auto inputs = circ->getInputNodeIDs();
        size_t currentCharPosition = 0;

        for (char binaryChar : numberToRepresent) {
            if (binaryChar == '0') {
                result[inputs[currentCharPosition]] = false;

            } else {
                result[inputs[currentCharPosition]] = true;
            }
            ++currentCharPosition;
        }

        return result;
    }

    std::string prepareOutput(const core::CircuitReadOnly *circ, const std::unordered_map<uint64_t, bool> &environment) {
        std::stringstream result;
        for (auto output : circ->getOutputNodeIDs()) {
            result << environment.at(output);
        }
        return result.str();
    }

    // ---------------------------------- TEST CASES ----------------------------------

    TEST(MNIST, TestProperties) {
        auto builder = generateSecureMLNN();
        fuse::core::ModuleContext context(builder);
        auto mod = context.getModuleBufferWrapper();
        auto mainCirc = mod.getEntryCircuit();

        std::unordered_map<uint64_t, std::vector<std::any>> env;
        for (auto inputID : mainCirc->getInputNodeIDs()) {
            env[inputID].push_back(10);
        }
        ASSERT_NO_THROW(fuse::backend::experimental::evaluate(mod, env));
        for (auto outputID : mainCirc->getOutputNodeIDs()) {
            ASSERT_TRUE(env.contains(outputID));
        }
    }

    TEST(GT32, DISABLED_BuildRelu) {
        const std::string gt32Path = "../../examples/bristol_circuits/int_gt32_depth.bristol";
        auto context = fuse::frontend::loadFUSEFromBristol(gt32Path);
        auto circ = context.getMutableCircuitWrapper();
        fuse::backend::PlaintextInterpreter<bool> interpreter;
        std::bitset<32> input1;
        std::bitset<32> input2;
        std::bitset<32> zero = 0;
        std::string threshold{"11111111111111111111111111111110"};

        // sanity check that comparison works
        input1 = 78;
        input2 = 15;
        auto env = prepareEnvironment(&circ, input1.to_string() + input2.to_string());
        ASSERT_NO_THROW(interpreter.evaluate(circ, env));
        EXPECT_STREQ(prepareOutput(&circ, env).c_str(), "10000000000000000000000000000000");

        std::vector<uint64_t> resNodes;
        std::unordered_set<uint64_t> outputNodesToDel(circ.getNumberOfOutputs());
        for (auto outputID : circ.getOutputNodeIDs()) {
            resNodes.push_back(circ.getNodeWithID(outputID).getInputNodeIDs()[0]);
            outputNodesToDel.insert(outputID);
        }
        // get result of comparison -> use it in AND gate with all the result values
        auto compBit = circ.getNodeWithID(285).getInputNodeIDs()[0];
        auto invBit = circ.addNode();
        invBit.setInputNodeID(compBit);
        invBit.setPrimitiveOperation(fuse::core::ir::PrimitiveOperation::Not);

        // construct AND and Output Nodes
        std::vector<uint64_t> newOutputIDs;
        auto inputNodes = circ.getInputNodeIDs();
        int index = 0;
        for (auto resID : resNodes) {
            auto andNode = circ.addNode();
            std::vector<uint64_t> ins{invBit.getNodeID(), inputNodes[index]};
            andNode.setInputNodeIDs(ins);
            andNode.setPrimitiveOperation(fuse::core::ir::PrimitiveOperation::And);

            auto outNode = circ.addNode();
            std::vector<uint64_t> out{andNode.getNodeID()};
            outNode.setInputNodeIDs(out);
            outNode.setPrimitiveOperation(fuse::core::ir::PrimitiveOperation::Output);
            newOutputIDs.push_back(outNode.getNodeID());
            ++index;
        }

        // circ.removeNodes(outputNodesToDel);
        circ.setOutputNodeIDs(newOutputIDs);

        env = prepareEnvironment(&circ, threshold + threshold);
        ASSERT_NO_THROW(interpreter.evaluate(circ, env));
        EXPECT_STREQ(prepareOutput(&circ, env).c_str(), threshold.c_str());

        env = prepareEnvironment(&circ, "00000000000000000000000000000001" + threshold);
        ASSERT_NO_THROW(interpreter.evaluate(circ, env));
        EXPECT_STREQ(prepareOutput(&circ, env).c_str(), "00000000000000000000000000000001");

        env = prepareEnvironment(&circ, "10000000000000000000000000000000" + threshold);
        ASSERT_NO_THROW(interpreter.evaluate(circ, env));
        EXPECT_STREQ(prepareOutput(&circ, env).c_str(), "10000000000000000000000000000000");

        env = prepareEnvironment(&circ, "00000000000000000000000000000110" + threshold);
        ASSERT_NO_THROW(interpreter.evaluate(circ, env));
        EXPECT_STREQ(prepareOutput(&circ, env).c_str(), "00000000000000000000000000000110");
    }

    TEST(GT32, DISABLED_Correctness) {
        const std::string gt32Path = "../../examples/bristol_circuits/int_gt32_depth.bristol";
        auto context = fuse::frontend::loadFUSEFromBristol(gt32Path);
        auto circ = context.getReadOnlyCircuit();

        fuse::backend::PlaintextInterpreter<bool> interpreter;
        std::bitset<32> input1;
        std::bitset<32> input2;
        std::string threshold{"11111111111111111111111111111110"};

        input1 = 15;
        input2 = 78;
        auto env = prepareEnvironment(circ.get(), input1.to_string() + input2.to_string());
        ASSERT_NO_THROW(interpreter.evaluate(*circ.get(), env));
        EXPECT_STREQ(prepareOutput(circ.get(), env).c_str(), "00000000000000000000000000000000");

        input1 = 78;
        input2 = 15;
        env = prepareEnvironment(circ.get(), input1.to_string() + input2.to_string());
        ASSERT_NO_THROW(interpreter.evaluate(*circ.get(), env));
        EXPECT_STREQ(prepareOutput(circ.get(), env).c_str(), "10000000000000000000000000000000");
        // std::cout << "output node ids: ";
        // for (auto output : circ->getOutputNodeIDs()) {
        //     std::cout << std::to_string(output);
        //     if (env.at(output) == 1) {
        //         std::cout << " (set to 1)";
        //     }
        //     std::cout << ", ";
        // }

        input1 = 78;
        input2 = 78;
        env = prepareEnvironment(circ.get(), input1.to_string() + input2.to_string());
        ASSERT_NO_THROW(interpreter.evaluate(*circ.get(), env));
        EXPECT_STREQ(prepareOutput(circ.get(), env).c_str(), "00000000000000000000000000000000");

        env = prepareEnvironment(circ.get(), input1.to_string() + threshold);
        ASSERT_NO_THROW(interpreter.evaluate(*circ.get(), env));
        EXPECT_STREQ(prepareOutput(circ.get(), env).c_str(), "00000000000000000000000000000000");

        // input1 = "01111111111111111111111111111111";
        input2 = 78;
        env = prepareEnvironment(circ.get(), threshold + input2.to_string());
        ASSERT_NO_THROW(interpreter.evaluate(*circ.get(), env));
        EXPECT_STREQ(prepareOutput(circ.get(), env).c_str(), "10000000000000000000000000000000");
    }

    TEST(MnistCirc, DISABLED_Structure) {
        const std::string output = "../../tests/outputs/optimizations/const_fold_booleans.txt";
        const std::string mnistPath = "../../examples/hycc_circuits/compiled_to_fuseir/mnist.mfs";

        fuse::core::ModuleContext context;
        context.readModuleFromFile(mnistPath);
        auto mod = context.getModuleBufferWrapper();
        auto circ = mod.getCircuitWithName("relu");
        auto relu = circ.get();

        fuse::backend::PlaintextInterpreter<bool> interpreter;
        // input IDs: [0;31]
        std::unordered_map<uint64_t, bool> inputMappings;
        std::bitset<32> input;

        //---------------------------------- check which endianness is used in relu ----------------------------------

        // check if relu (0000 ... 0001) > 0
        input = 1;
        auto env = prepareEnvironment(relu, input.to_string());
        ASSERT_NO_THROW(interpreter.evaluate(*relu, env));
        EXPECT_STREQ(prepareOutput(relu, env).c_str(), "00000000000000000000000000000001");

        // check if relu (1000 ... 0000) > 0
        input = 1 << 31;
        env = prepareEnvironment(relu, input.to_string());
        ASSERT_NO_THROW(interpreter.evaluate(*relu, env));
        EXPECT_STREQ(prepareOutput(relu, env).c_str(), "10000000000000000000000000000000");

        for (int64_t testVal = UINT32_MAX - 100; testVal <= UINT32_MAX; ++testVal) {
            input = testVal;
            env = prepareEnvironment(relu, input.to_string());
            ASSERT_NO_THROW(interpreter.evaluate(*relu, env));
            EXPECT_STREQ(prepareOutput(relu, env).c_str(), input.to_string().c_str());
        }
    }
} // namespace fuse::tests::passes
