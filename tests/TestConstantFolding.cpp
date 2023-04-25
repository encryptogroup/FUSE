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

#include "BristolFrontend.h"
#include "ConstantFolder.h"
#include "DOTBackend.h"
#include "DeadNodeEliminator.h"
#include "ModuleBuilder.h"

namespace fuse::tests::passes {
TEST(ConstantFolder, BooleanConstants) {
    const std::string output = "../../tests/outputs/optimizations/const_fold_booleans.txt";
    std::ofstream of(output);

    fuse::frontend::CircuitBuilder circuitBuilder("testBoolean1");
    auto inputBoolType = circuitBuilder.addDataType(fuse::core::ir::PrimitiveType::Bool);
    auto outputBoolType = circuitBuilder.addDataType(fuse::core::ir::PrimitiveType::Bool, fuse::core::ir::SecurityLevel::Plaintext);

    auto in1 = circuitBuilder.addInputNode({inputBoolType});
    auto in2 = circuitBuilder.addInputNode({inputBoolType});

    auto c1 = circuitBuilder.addConstantNodeWithPayload(true);
    auto and1 = circuitBuilder.addNode(fuse::core::ir::PrimitiveOperation::And, {in1, in2, c1});
    circuitBuilder.addOutputNode({outputBoolType}, {and1});

    auto c2 = circuitBuilder.addConstantNodeWithPayload(false);
    auto xor1 = circuitBuilder.addNode(fuse::core::ir::PrimitiveOperation::Xor, {c1, c2});
    auto c3 = circuitBuilder.addConstantNodeWithPayload(true);
    auto and2 = circuitBuilder.addNode(fuse::core::ir::PrimitiveOperation::Xor, {xor1, c3});
    circuitBuilder.addOutputNode({outputBoolType}, {and2});
    circuitBuilder.finish();

    fuse::core::CircuitContext context(circuitBuilder);
    auto wrapper = context.getMutableCircuitWrapper();
    of << fuse::backend::generateDotCodeFrom(wrapper);
    of.flush();
    of << "\nOptimized:\n";

    fuse::passes::foldConstantNodes(wrapper);
    fuse::passes::eliminateDeadNodes(wrapper);
    of << fuse::backend::generateDotCodeFrom(wrapper);
    of.flush();
}

}  // namespace fuse::tests::passes
