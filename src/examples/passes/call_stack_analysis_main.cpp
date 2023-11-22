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
#include <filesystem>
#include <fstream>
#include <iostream>

#include "IR.h"
#include "CallStackAnalysis.h"
#include "OperationsAnalysis.h"
#include "DOTBackend.h"

int main(int argc, char *argv[]) {
    const std::string hyccCirc = "../../examples/hycc_circuits/compiled_to_fuseir/mnist.mfs";
    const std::string output = "../../tests/outputs/callAnalysis.txt";
    const std::string relu_dot_output = "../../tests/outputs/relu_dot.txt";
    std::ofstream of(output);
    std::ofstream relu_of(relu_dot_output);

    fuse::core::ModuleContext context;
    context.readModuleFromFile(hyccCirc);
    auto mod = context.getModuleBufferWrapper();

    auto relu = mod.getCircuitWithName("relu");
    relu_of << fuse::backend::generateDotCodeFrom(*relu) << std::endl;

    std::unordered_map<std::string, std::unordered_map<std::string, int>> res = fuse::passes::analyzeCallStacks(mod);
    std::unordered_map<std::string, std::unordered_map<std::string, int>> ops = fuse::passes::analyzeOperations(mod);

    for (const auto &pair : res) {
        of << "circuit: " << pair.first; // pair.first is the current circuit
        of << "\n\t[operations]: ";
        for (const auto &op : ops.at(pair.first)) {
            of << "\n\t\t";
            of << op.first << " : " << op.second << " ; ";
        }
        of << "\n\t[callees]: ";
        for (const auto &callee : pair.second) {
            of << "\n\t\t";
            of << callee.first << " : " << callee.second << " ; ";
        }
        of << "\n\n";
    }

    of.flush();
}
