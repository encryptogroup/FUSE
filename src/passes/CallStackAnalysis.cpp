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

#include "CallStackAnalysis.h"

namespace fuse::passes {

std::unordered_map<std::string, int> analyzeCircuitCallStacks(const core::CircuitReadOnly& circ) {
    std::unordered_map<std::string, int> calc_res;

    circ.topologicalTraversal([&](const core::NodeReadOnly& node){
        if (node.isSubcircuitNode()) {
            const auto name = node.getSubCircuitName();
            calc_res[name]++;
        }
    });
    return calc_res;
}

std::unordered_map<std::string, std::unordered_map<std::string, int>> analyzeCallStacks(const core::ModuleReadOnly& mod) {
    std::unordered_map<std::string, std::unordered_map<std::string, int>> result;

    const auto& circs = mod.getAllCircuitNames();
    for (const auto& circName : circs) {
        const auto& key = mod.getCircuitWithName(circName);
        const auto val = analyzeCircuitCallStacks(*key);
        result[key->getName()] = val;
    }

    return result;
}

} // namespace fuse::passes
