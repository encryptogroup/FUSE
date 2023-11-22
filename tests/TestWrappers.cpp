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

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <regex>

#include "BristolFrontend.h"
#include "HyCCFrontend.h"
#include "IR.h"
#include "ModuleBuilder.h"
#include "ModuleWrapper.h"

namespace fuse::tests::core {

namespace fe = fuse::frontend;
namespace ir = fuse::core::ir;
namespace core = fuse::core;

const std::string bristolCircs = "../../examples/bristol_circuits";
const std::string outputDir = "../../tests/outputs/wrapper_test_circuits";

TEST(TestWrappers, DataTypeWrapper) {
    // create circuit with only one constant node
    fe::CircuitBuilder builder("main");
    auto constant = builder.addConstantNodeWithPayload(false);
    auto id = builder.addDataType(ir::PrimitiveType::Bool, ir::SecurityLevel::Plaintext, {1, 2, 3}, "test");
    auto input = builder.addInputNode({id}, "");

    builder.finish();
    ASSERT_NE(constant, input);

    // read circuit and call wrapper functions
    auto circWrapper = core::CircuitBufferWrapper(builder.getSerializedCircuitBufferPointer());

    ASSERT_EQ(circWrapper.getNumberOfNodes(), 2);
    ASSERT_EQ(circWrapper.getNumberOfInputs(), 1);
    ASSERT_EQ(circWrapper.getNumberOfOutputs(), 0);
    size_t count = 0;
    for (auto node : circWrapper) {
        ++count;
    }
    ASSERT_EQ(count, 2);

    // test nodes
    auto c = circWrapper.getNodeWithID(constant);
    ASSERT_EQ(c->getOperation(), ir::PrimitiveOperation::Constant);
    ASSERT_FALSE(c->getConstantFlexbuffer().IsNull());
    ASSERT_FALSE(circWrapper.getNodeWithID(constant)->getConstantBool());
    ASSERT_EQ(circWrapper.getNodeWithID(input)->getOperation(), ir::PrimitiveOperation::Input);

    // test datatype
    auto inputs = circWrapper.getInputDataTypes();
    auto& dtWrapper = inputs.at(0);
    ASSERT_EQ(dtWrapper->getPrimitiveType(), ir::PrimitiveType::Bool);
    ASSERT_EQ(dtWrapper->getSecurityLevel(), ir::SecurityLevel::Plaintext);
    ASSERT_STREQ(dtWrapper->getDataTypeAnnotations().c_str(), "test");
    auto sp = dtWrapper->getShape();
    ASSERT_EQ(sp.size(), 3);
    ASSERT_EQ(sp[0], 1);
    ASSERT_EQ(sp[1], 2);
    ASSERT_EQ(sp[2], 3);
}

TEST(TestWrappers, Context) {
    core::CircuitContext context = frontend::loadFUSEFromBristol(bristolCircs + "/fullAdder.bristol");
    {
        auto circ = context.getCircuitBufferWrapper();
        std::cout << circ.getName() << std::endl;
    }
    auto mutableCirc = context.getMutableCircuitWrapper();
    std::cout << mutableCirc.getName();
}

TEST(TestWrappers, RemoveCircuitFromModuleWhenUnpacking) {
    // TODO needs to be tested when we have anything that uses modules
}

}  // namespace fuse::tests::core
