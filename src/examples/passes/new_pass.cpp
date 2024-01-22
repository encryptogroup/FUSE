/*
 * MIT License
 *
 * Copyright (c) 2024 Nora Khayata
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

// This includes the top-level objects for FUSE IR: ModuleContext and CircuitContext
#include "IR.h"
#include "ModuleGenerator.h"
// prints DOT code of your FUSE IR
#include "DOTBackend.h"
// Here include your own analysis and/or transformation passes
// As an example, we include two analyses:
// (1) An analysis that returns the overall occurrence of any operation in a circuit
#include "OperationsAnalysis.h"
// (2) an analysis that returns for every node on which depth it is w.r.t to nodes of the same operation

int main(int argc, char *argv[])
{
    // put path to circuit/module here if you want to load it
    // otherwise call the function that returns a ModuleContext or CircuitContext for self generated stuff
    fuse::core::CircuitContext ctx = fuse::util::generateCircuitWithNumberOfNodes(1000);
    fuse::core::CircuitBufferWrapper circ = ctx.getCircuitBufferWrapper();

    // make sure the directory tests/outputs exists, otherwise nothing will happen here
    const std::string analysis_output = "../../tests/outputs/callAnalysis.txt";
    const std::string dot_output = "../../tests/outputs/dot.txt";
    std::ofstream of(analysis_output);
    std::ofstream dot_of(dot_output);

    dot_of << fuse::backend::generateDotCodeFrom(circ) << std::endl;

    std::unordered_map<std::string, int> ops = fuse::passes::analyzeOperations(circ);

    for (const auto &pair : ops)
    {
        of << "Operation: [" << pair.first << "] --- Occurence: " << pair.second << " times" << std::endl;
    }
}
