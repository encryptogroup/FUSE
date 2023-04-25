/*
 * MIT License
 *
 * Copyright (c) 2022 Moritz Huppert
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
#include "DOTBackend.h"
#include "InstructionVectorization.h"

namespace fuse::tests::passes {

TEST(InstructionVectorization, simple) {
    //const std::string optimizable = "../../tests/resources/optimizable/vectorgraph.txt";
    const std::string optimizable = "../../examples/bristol_circuits/int_add8_size.bristol";
    const std::string output = "../../tests/outputs/optimizations/vectorgraph.txt";
    std::ofstream of(output);

    auto context = fuse::frontend::loadFUSEFromBristol(optimizable);
    auto circ = context.getMutableCircuitWrapper();
    of << fuse::backend::generateDotCodeFrom(circ);
    of << "\nVectorized:\n";
    // fuse::passes::vectorizeInstructons(circ, core::ir::PrimitiveOperation::Xor);
    fuse::passes::vectorizeInstructions(circ, core::ir::PrimitiveOperation::Xor, 2, 10);
    of << fuse::backend::generateDotCodeFrom(circ);
    of.flush();
}

TEST(CompleteInstructionVectorization, simple) {
    //const std::string optimizable = "../../tests/resources/optimizable/vectorgraph.txt";
    const std::string optimizable = "../../examples/bristol_circuits/int_add8_depth.bristol";
    const std::string output = "../../tests/outputs/optimizations/multivectorgraph.txt";
    std::ofstream of(output);

    auto context = fuse::frontend::loadFUSEFromBristol(optimizable);
    auto circ = context.getMutableCircuitWrapper();
    of << fuse::backend::generateDotCodeFrom(circ);
    of << "\nVectorized:\n";
    // fuse::passes::vectorizeInstructons(circ, core::ir::PrimitiveOperation::Xor);
    fuse::passes::vectorizeAllInstructions(circ);
    of << fuse::backend::generateDotCodeFrom(circ);
    of.flush();
}

}  // namespace fuse::tests::passes
