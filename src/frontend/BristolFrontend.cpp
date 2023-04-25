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

#include "BristolFrontend.h"

#include <fstream>
#include <iostream>

#include "ModuleBuilder.h"
#include "ModuleWrapper.h"

namespace fuse::frontend {

CircuitBuilder buildFUSEFromBristol(const std::string &bristolInputFilePath) {
    std::ifstream inputFile;
    inputFile.open(bristolInputFilePath);
    if (!inputFile) {
        throw bristol_error("File " + bristolInputFilePath + " could not be opened");
    }

    auto base_name = bristolInputFilePath.substr(bristolInputFilePath.find_last_of("/") + 1);
    auto dot = base_name.find('.');
    auto name = base_name.substr(0, dot);
    CircuitBuilder circuitBuilder(name);

    /*
    if successfully opened, read out line by line
    */
    std::string line;

    int numOfGates, numOfWires;
    // first line: number of gates, number of wires
    while (std::getline(inputFile, line)) {
        if (line.empty()) {
            continue;
        } else {
            std::istringstream iss(line);
            iss >> numOfGates >> numOfWires;
            break;
        }
    }

    // second line: number of input wires for both parties and number of output wires
    short numOfInputWiresFromPartyOne, numOfInputWiresFromPartyTwo, totalNumOfOutputWires;
    std::vector<Identifier> nodesToOutput;

    while (std::getline(inputFile, line)) {
        if (line.empty()) {
            continue;
        } else {
            std::istringstream iss(line);
            iss >> numOfInputWiresFromPartyOne >> numOfInputWiresFromPartyTwo >> totalNumOfOutputWires;
            break;
        }
    }

    // Address Spaces for wires: Input Wires from party 1/2, Intermediate Wires, Output Wires

    int in1Begin = 0;
    int in1End = numOfInputWiresFromPartyOne - 1;
    int in2Begin = numOfInputWiresFromPartyOne;
    int in2End = in2Begin + numOfInputWiresFromPartyTwo - 1;
    int outBegin = numOfWires - totalNumOfOutputWires;

    nodesToOutput.reserve(totalNumOfOutputWires);

    auto secureBooleanType = circuitBuilder.addDataType(ir::PrimitiveType::Bool, ir::SecurityLevel::Secure);
    auto plaintextBooleanType = circuitBuilder.addDataType(ir::PrimitiveType::Bool, ir::SecurityLevel::Plaintext);

    //         input nodes for party 1
    for (int i = in1Begin; i <= in1End; i++) {
        circuitBuilder.addInputNode(i, secureBooleanType, "owner:1");
    }

    //         input nodes for party 2
    for (int i = in2Begin; i <= in2End; i++) {
        circuitBuilder.addInputNode(i, secureBooleanType, "owner:2");
    }

    // succeeding lines: read inputs, output and gate operation
    while (std::getline(inputFile, line)) {
        if (line.empty()) {
            continue;
        } else {
            short numOfInputWires, numOfOutputWires;
            std::istringstream iss(line);

            iss >> numOfInputWires >> numOfOutputWires;

            assert(numOfInputWires == 1 || numOfInputWires == 2);
            assert(numOfOutputWires == 1);

            unsigned long outGate;

            if (numOfInputWires == 2) {
                unsigned long inGate1, inGate2;
                std::string binaryOperand;
                iss >> inGate1 >> inGate2 >> outGate >> binaryOperand;
                assert((binaryOperand == "AND") || (binaryOperand == "XOR") || (binaryOperand == "OR"));

                if (binaryOperand == "AND") {
                    circuitBuilder.addNode(outGate, ir::PrimitiveOperation::And, {inGate1, inGate2});
                } else if (binaryOperand == "XOR") {
                    circuitBuilder.addNode(outGate, ir::PrimitiveOperation::Xor, {inGate1, inGate2});
                } else if (binaryOperand == "OR") {
                    circuitBuilder.addNode(outGate, ir::PrimitiveOperation::Or, {inGate1, inGate2});
                }

            } else if (numOfInputWires == 1) {
                unsigned long inGate;
                std::string unaryOperand;
                iss >> inGate >> outGate >> unaryOperand;
                assert(unaryOperand == "INV");
                circuitBuilder.addNode(outGate, ir::PrimitiveOperation::Not, {inGate});
            }

            if (outGate >= outBegin) {
                nodesToOutput.push_back(outGate);
            }
        }
    }

    std::sort(nodesToOutput.begin(), nodesToOutput.end());
    size_t offset = 0;
    for (auto toOutput : nodesToOutput) {
        circuitBuilder.addOutputNode(numOfWires + offset, plaintextBooleanType, {toOutput});
        ++offset;
    }

    // close bristol input file
    inputFile.close();
    return circuitBuilder;
}

// core::ModuleBufferWrapper loadFUSEFromBristol(const std::string &bristolInputFilePath) {
//     auto circuitBuilder = buildFUSEFromBristol(bristolInputFilePath);
// }

void loadFUSEFromBristolToFile(const std::string &bristolInputFilePath, const std::string &outputBufferPath) {
    auto circuitBuilder = buildFUSEFromBristol(bristolInputFilePath);
    circuitBuilder.finishAndWriteToFile(outputBufferPath);
}

core::CircuitContext loadFUSEFromBristol(const std::string &bristolInputFilePath) {
    auto circuitBuilder = buildFUSEFromBristol(bristolInputFilePath);
    circuitBuilder.finish();
    core::CircuitContext resultContext(circuitBuilder);
    return resultContext;
}

}  // namespace fuse::frontend
