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

#include <IOHandlers.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <bitset>
#include <climits>
#include <filesystem>
#include <fstream>
#include <gzip/compress.hpp>
#include <random>
#include <regex>
#include <string>

#include "BristolFormatBackend.h"
#include "BristolFrontend.h"
#include "DOTBackend.h"
#include "HyCCFrontend.h"
#include "IR.h"
#include "PlaintextInterpreter.hpp"

namespace fuse::tests::frontend {
std::unordered_map<uint64_t, bool> prepareEnvironment(const core::CircuitReadOnly& circ, std::string numberToRepresent) {
    auto in = circ.getNumberOfInputs();
    auto size = numberToRepresent.size();
    assert(in == size);
    std::unordered_map<uint64_t, bool> result;
    auto inputs = circ.getInputNodeIDs();
    size_t currentCharPosition = 0;

    for (char binaryChar : numberToRepresent) {
        if (binaryChar == '0') {
            result[inputs[currentCharPosition]] = false;

        } else {
            result[inputs[currentCharPosition]] = true;
        }
        ++currentCharPosition;
    }

    return result;
}

std::string prepareOutput(const core::CircuitReadOnly& circ, const std::unordered_map<uint64_t, bool>& environment) {
    std::stringstream result;
    for (auto output : circ.getOutputNodeIDs()) {
        result << environment.at(output);
    }
    return result.str();
}

TEST(HyCCFrontendTest, EuclideanTutorial) {
    namespace io = fuse::core::util::io;
    namespace frontend = fuse::frontend;

    auto hycc = "../../extern/HyCC/examples/tutorial_euclidean_distance/";
    auto bristol = "../../extern/HyCC/examples/tutorial_euclidean_distance/tutorial_euclidean_distance.txt";
    auto fuseFromBristol = "../../extern/HyCC/examples/tutorial_euclidean_distance/tutorial_euclidean_distance.fs";
    auto fuseFromHyCC = "../../extern/HyCC/examples/tutorial_euclidean_distance/tutorial_euclidean_distance_hycc.fs";
    auto dotOutput = "../../extern/HyCC/examples/tutorial_euclidean_distance/tutorial_euclidean_distance_hycc.dot";

    fuse::backend::PlaintextInterpreter<bool> interpreter;
    std::bitset<64> in1 = 0;
    std::bitset<64> in2 = 0;

    ASSERT_NO_THROW(frontend::loadFUSEFromBristolToFile(bristol, fuseFromBristol));
    fuse::core::CircuitContext context;
    const core::CircuitBufferWrapper bristolCirc = context.readCircuitFromFile(fuseFromBristol);

    ASSERT_NO_THROW(fuse::frontend::loadFUSEFromHyCC(hycc, fuseFromHyCC));
    fuse::core::ModuleContext modContext;
    core::ModuleBufferWrapper hyccMod = modContext.readModuleFromFile(fuseFromHyCC);
    auto hyccCirc = hyccMod.getEntryCircuit();

    // test some random values to check if both bristol and hycc work the same
    srand(time(NULL));
    std::random_device rd;
    std::mt19937_64 e2(rd());
    std::uniform_int_distribution<long long int> dist(std::llround(std::pow(2, 61)), std::llround(std::pow(2, 62)));
    for (int i = 0; i < 20; ++i) {
        // generate random input 32 bit numbers
        in1 = dist(e2);
        in2 = dist(e2);

        // compute addition using the bristol circuit
        auto env1 = prepareEnvironment(bristolCirc, in1.to_string() + in2.to_string());
        ASSERT_NO_THROW(interpreter.evaluate(bristolCirc, env1));
        auto out1 = prepareOutput(bristolCirc, env1);

        // compute addition using the HyCC circuit
        auto env2 = prepareEnvironment(*hyccCirc, in1.to_string() + in2.to_string());
        ASSERT_NO_THROW(interpreter.evaluate(*hyccCirc, env2));
        auto out2 = prepareOutput(*hyccCirc, env2);

        // check that the outputs of hycc and bristol are equal
        ASSERT_EQ(out1.size(), out2.size());
        ASSERT_STREQ(out1.c_str(), out2.c_str());
    }
}

TEST(HyCCFrontendTest, AdditionTutorial) {
    namespace io = fuse::core::util::io;
    namespace frontend = fuse::frontend;

    auto hycc = "../../extern/HyCC/examples/tutorial_addition/";
    auto bristol = "../../extern/HyCC/examples/tutorial_addition/tutorial_addition.txt";
    auto fuseFromBristol = "../../extern/HyCC/examples/tutorial_addition/tutorial_addition.fs";
    auto fuseFromHyCC = "../../extern/HyCC/examples/tutorial_addition/tutorial_addition_hycc.fs";
    auto dotOutput = "../../extern/HyCC/examples/tutorial_addition/tutorial_addition_hycc.dot";

    fuse::backend::PlaintextInterpreter<bool> interpreter;
    std::bitset<32> in1 = 0;
    std::bitset<32> in2 = 0;

    // bristol: 32, 32 -> 32
    ASSERT_NO_THROW(frontend::loadFUSEFromBristolToFile(bristol, fuseFromBristol));
    fuse::core::CircuitContext context;
    const core::CircuitBufferWrapper bristolCirc = context.readCircuitFromFile(fuseFromBristol);

    // hycc: 8 inputs
    ASSERT_NO_THROW(fuse::frontend::loadFUSEFromHyCC(hycc, fuseFromHyCC));
    core::ModuleContext modContext;
    const core::ModuleBufferWrapper hyccMod = modContext.readModuleFromFile(fuseFromHyCC);
    auto hyccCirc = hyccMod.getEntryCircuit();

    // test some random values to check if both bristol and hycc work the same
    srand(time(NULL));
    for (int i = 0; i < 20; ++i) {
        // generate random input 32 bit numbers
        in1 = std::rand();
        in2 = std::rand();

        // compute addition using the bristol circuit
        auto env1 = prepareEnvironment(bristolCirc, in1.to_string() + in2.to_string());
        ASSERT_NO_THROW(interpreter.evaluate(bristolCirc, env1));
        auto out1 = prepareOutput(bristolCirc, env1);

        // compute addition using the HyCC circuit
        auto env2 = prepareEnvironment(*hyccCirc, in1.to_string() + in2.to_string());
        ASSERT_NO_THROW(interpreter.evaluate(*hyccCirc, env2));
        auto out2 = prepareOutput(*hyccCirc, env2);

        // check that the outputs of hycc and bristol are equal
        ASSERT_STREQ(out1.c_str(), out2.c_str());
    }
}

TEST(HyCCFrontendTest, CompareFileSizes) {
    namespace io = fuse::core::util::io;
    namespace frontend = fuse::frontend;

    std::vector<std::string> hyccExamples = {
        "../../extern/HyCC/examples/tutorial_addition",            // works
        "../../extern/HyCC/examples/tutorial_euclidean_distance",  // works
        "../../extern/HyCC/examples/benchmarks/gauss",             // works
        "../../examples/hycc_circuits/biomatch1k",                 // works

        // "../../examples/hycc_circuits/cryptonets",  // exits before HyCC finishes loading
        // "../../examples/hycc_circuits/kmeans",      // exits before HyCC finishes loading
        // "../../examples/hycc_circuits/mnist",       // exits before HyCC finishes loading
    };

    std::string outputPath = "../../tests/outputs/hycc_frontend/";

    std::ofstream csvOutput;

    time_t t = time(nullptr);  // get time now
    struct tm* now = localtime(&t);
    char buffer[80];
    strftime(buffer, 80, "%d-%m-%y:%T", now);

    csvOutput.open(outputPath + "hycc_output_" + buffer + ".csv");
    csvOutput << "Name, Sum of all HyCC Circuits, FUSE without calls, FUSE with calls, (TODO) Bristol Size\n";

    for (const auto& hyccExample : hyccExamples) {
        auto const pos = hyccExample.find_last_of('/');
        const auto name = hyccExample.substr(pos + 1);

        // compute the sum of the size of all generated .circ files
        size_t hyccSize = 0;
        for (const auto& entry : std::filesystem::directory_iterator(hyccExample)) {
            if (entry.exists() && !entry.path().empty() && entry.path().extension() == ".circ") {
                hyccSize += std::filesystem::file_size(entry.path());
            }
        }

        std::string fusePath = outputPath + name + ".fs";
        std::string fusePathWithCalls = outputPath + name + "-calls.fs";
        std::string bristolPath = outputPath + name + ".bristol";

        // translate HyCC -> FUSE binary and calculate the binary size
        ASSERT_NO_THROW(fuse::frontend::loadFUSEFromHyCC(hyccExample, fusePath));
        auto fuseSizeWithoutCalls = std::filesystem::file_size(fusePath);
        ASSERT_NO_THROW(fuse::frontend::loadFUSEFromHyCCAndSaveToFile(hyccExample, fusePathWithCalls));
        auto fuseSizeWithCalls = std::filesystem::file_size(fusePathWithCalls);
        core::ModuleContext mc;
        mc.readModuleFromFile(fusePathWithCalls);
        auto mw = mc.getModuleBufferWrapper();
        auto all_circs = mw.getAllCircuitNames();

        // ASSERT_NO_THROW(fuse::core::util::io::writeStringToTextFile(bristolPath, fuse::backend::generateBristolFormatFrom(mc.getModuleBufferWrapper())));
        // auto bristolSize = std::filesystem::file_size(bristolPath);

        // write all the calculated sizes to the csv file
        csvOutput << name << ",";
        csvOutput << hyccSize << ",";
        csvOutput << fuseSizeWithoutCalls << ",";
        csvOutput << fuseSizeWithCalls << ",";
        // csvOutput << bristolSize << ",";
        csvOutput.flush();
    }

    csvOutput.close();
}

}  // namespace fuse::tests::frontend
