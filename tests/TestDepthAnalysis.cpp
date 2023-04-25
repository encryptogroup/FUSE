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
#include "DepthAnalysis.h"

namespace fuse::tests::passes {

TEST(DepthAnalysis, simpleDepth) {
    const std::vector<std::string> optimizable = {"../../tests/resources/subgraph/subgraph.txt"};
    const std::string output = "../../tests/outputs/optimizations/simpleDepth.txt";
    std::ofstream of(output);

    for (const auto test : optimizable) {
        auto context = fuse::frontend::loadFUSEFromBristol(test);
        auto circ = context.getMutableCircuitWrapper();
        of << fuse::backend::generateDotCodeFrom(circ);
        of << "\nDepth:\n";
        auto depthMap = fuse::passes::getNodeDepths(circ);
        //fuse::passes::replaceFrequentSubcircuits(circ, false, 3, 7);
        for(auto depthPair: depthMap){
            of << "Node with ID " << depthPair.first << " has depth " << depthPair.second  << ".\n";
        }
    }
}

TEST(DepthAnalysis, instructionDepth) {
    const std::vector<std::string> optimizable = {"../../tests/resources/subgraph/subgraph.txt"};
    const std::string output = "../../tests/outputs/optimizations/instructionDepth.txt";
    std::ofstream of(output);

    for (const auto test : optimizable) {
        auto context = fuse::frontend::loadFUSEFromBristol(test);
        auto circ = context.getMutableCircuitWrapper();
        of << fuse::backend::generateDotCodeFrom(circ);
        of << "\nDepth:\n";
        auto depthMap = fuse::passes::getNodeInstructionDepths(circ, fuse::core::ir::PrimitiveOperation::And);
        //fuse::passes::replaceFrequentSubcircuits(circ, false, 3, 7);
        for(auto depthPair: depthMap){
            of << "Node with ID " << depthPair.first << " has instruction depth " << depthPair.second << ".\n";
        }
    }
}

}  // namespace fuse::tests::passes
