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
#include "GraphBackend.h"

namespace fuse::tests::passes {

TEST(GraphBackend, DistgraphTestcase) {
    const std::vector<std::string> optimizable = {"../../tests/resources/subgraph/subgraph.txt"};
    const std::string output = "../../tests/outputs/optimizations/graph_Distgraph.txt";
    std::ofstream of(output);

    for (const auto test : optimizable) {
        auto context = fuse::frontend::loadFUSEFromBristol(test);
        auto circ = context.getMutableCircuitWrapper();
        of << fuse::backend::generateDotCodeFrom(circ);
        of << "\nGraph:\n";
        of << fuse::backend::generateDistgraphFrom(circ);
        of.flush();
    }
}

TEST(GraphBackend, GlasgowTestcase) {
    const std::vector<std::string> optimizable = {"../../tests/resources/subgraph/subgraph.txt"};
    const std::string output = "../../tests/outputs/optimizations/graph_Glasgow.txt";
    std::ofstream of(output);

    for (const auto test : optimizable) {
        auto context = fuse::frontend::loadFUSEFromBristol(test);
        auto circ = context.getMutableCircuitWrapper();
        of << fuse::backend::generateDotCodeFrom(circ);
        of << "\nGraph:\n";
        of << fuse::backend::generateGlasgowgraphFrom(circ);
        of.flush();
    }
}

TEST(GraphBackend, DistgraphToGlasgow) {
    const std::string optimizable = "../../tests/resources/graph_translate/distgraph_patterns.txt";
    const std::string output = "../../tests/outputs/optimizations/distgraphToGlasgow.txt";
    std::ofstream of(output);
    std::ifstream inf(optimizable, std::ios::in);
    std::string cur_line;

    while(std::getline(inf, cur_line)){
        of << "next Pattern:\n";
        of << cur_line;
        of << "\n";
        of << fuse::backend::translateDistgraphToGlasgow(cur_line);
        of << "\n";
        of << "\n";
    }
    of.flush();
}

}  // namespace fuse::tests::passes
