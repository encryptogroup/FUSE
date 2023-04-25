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
#include "FrequentSubcircuitReplacement.h"

namespace fuse::tests::passes {

TEST(FrequentSubcircuitReplacement, SimpleTestCases) {
    const std::vector<std::string> optimizable = {"../../tests/resources/subgraph/subgraph.txt"};
    const std::string output = "../../tests/outputs/optimizations/frequent_replacement_simple.txt";
    std::ofstream of(output);

    for (const auto test : optimizable) {
        auto context = fuse::frontend::loadFUSEFromBristol(test);
        auto circ = context.getMutableCircuitWrapper();
        of << fuse::backend::generateDotCodeFrom(circ);
        of << "\nOptimized:\n";
        auto moduleContext = fuse::passes::replaceFrequentSubcircuits(context, 14);
        // auto moduleContext = fuse::passes::replaceFrequentSubcircuits(context, 60);
        of << fuse::backend::generateDotCodeFrom(*moduleContext.getReadOnlyModule()->getEntryCircuit());
        of.flush();
    }
}

TEST(AFSR, SimpleTestCases) {
    const std::vector<std::string> optimizable = {"../../examples/bristol_circuits/int_mul16_size.bristol"};
    //const std::vector<std::string> optimizable = {"../../tests/resources/subgraph/subgraph.txt"};
    const std::string output = "../../tests/outputs/optimizations/automatic_frequent_replacement_simple.txt";
    std::ofstream of(output);

    for (const auto test : optimizable) {
        auto context = fuse::frontend::loadFUSEFromBristol(test);
        auto circ = context.getMutableCircuitWrapper();
        of << fuse::backend::generateDotCodeFrom(circ);
        of << "\nOptimized:\n";
        //auto moduleContext = fuse::passes::automaticallyReplaceFrequentSubcircuits(context, 1, 3, 10000, 2);
        auto moduleContext = fuse::passes::automaticallyReplaceFrequentSubcircuits(context);
        of << fuse::backend::generateDotCodeFrom(*moduleContext.getReadOnlyModule()->getEntryCircuit());
        of.flush();
    }
}

}  // namespace fuse::tests::passes
