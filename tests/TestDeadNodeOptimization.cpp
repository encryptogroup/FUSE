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
#include "DOTBackend.h"
#include "DeadNodeEliminator.h"

namespace fuse::tests::passes {

TEST(DeadNodeEliminator, SimpleTestCases) {
    const std::vector<std::string> optimizable = {"../../tests/resources/optimizable/dead_nodes_1.txt",
                                                  "../../tests/resources/optimizable/dead_nodes_2.txt"};
    const std::string output = "../../tests/outputs/optimizations/dead_nodes_simple.txt";
    std::ofstream of(output);

    for (const auto test : optimizable) {
        auto context = fuse::frontend::loadFUSEFromBristol(test);
        auto circ = context.getMutableCircuitWrapper();
        of << fuse::backend::generateDotCodeFrom(circ);
        of << "\nOptimized:\n";
        fuse::passes::eliminateDeadNodes(circ);
        of << fuse::backend::generateDotCodeFrom(circ);
        of.flush();
    }
}

}  // namespace fuse::tests::passes
