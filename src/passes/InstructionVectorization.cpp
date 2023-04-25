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

#include "InstructionVectorization.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "DOTBackend.h"
#include "DepthAnalysis.h"

namespace fuse::passes {

void vectorizeInstructions(core::CircuitObjectWrapper& circuit, core::ir::PrimitiveOperation operationType, int minGates, int maxDistance, bool multi) {
    // prepare tmp folder
    namespace fs = std::filesystem;
    const std::string output_dir = "../../tmp/";
    if (!fs::exists(output_dir)) {
        fs::create_directory(output_dir);
    }

    // eval variables
    int replaced = 0;
    int replaced_calls = 0;

    // prepare report
    std::string report_str;
    std::ofstream report;
    if (multi) {
        report_str = output_dir + "MIVreport.txt";
        report.open(report_str, std::ios::app);
    } else {
        report_str = output_dir + "IVreport.txt";
        report.open(report_str, std::ios::trunc);
    }
    report << "Replacing gates of type: " << fuse::core::ir::EnumNamePrimitiveOperation(operationType) << std::endl;
    report << "Circuit size before vec: " << circuit.getNumberOfNodes() << std::endl;
    report.flush();

    // apply instruction depth analysis
    report << "\n"
           << "Starting instruction depth analysis " << std::endl;
    std::unordered_map<uint64_t, uint64_t> instructionDepth = fuse::passes::getNodeInstructionDepths(circuit, operationType);

    // apply normal depth analysis
    report << "Starting node depth analysis " << std::endl;
    std::unordered_map<uint64_t, uint64_t> nodeDepth = fuse::passes::getNodeDepths(circuit);

    // remove all but the gates of the specific type
    std::map<uint64_t, std::vector<uint64_t>> depthToNode;
    for (auto nodePair : instructionDepth) {
        auto curNode = circuit.getNodeWithID(nodePair.first);
        if (curNode.getOperation() == operationType) {
            depthToNode[nodePair.second].push_back(nodePair.first);
        }
    }

    int ctr = 1;
    for (auto depthPair : depthToNode) {
        int size = depthPair.second.size();
        if (ctr % 100 == 0) {
            report << "Replacing candidate list " << ctr << "/" << depthToNode.size() << std::endl;
        }

        if (size >= minGates) {
            // calculate median
            std::vector<int> depth_vec;
            for (auto entry : depthPair.second) {
                depth_vec.push_back(nodeDepth[entry]);
            }
            std::sort(depth_vec.begin(), depth_vec.end());

            int median_depth = 0;

            if (depth_vec.size() % 2 == 0) {
                median_depth = (depth_vec[depth_vec.size() / 2 - 1] + depth_vec[depth_vec.size() / 2]) / 2;
            } else {
                median_depth = depth_vec[depth_vec.size() / 2];
            }

            // remove if distance too large
            std::vector<uint64_t> final_vec;
            for (auto node : depthPair.second) {
                if (abs(median_depth - nodeDepth[node]) <= maxDistance) {
                    final_vec.push_back(node);
                }
            }

            int new_size = final_vec.size();
            if (new_size >= minGates) {
                circuit.replaceNodesBySIMDNode(final_vec);
                // std::string deb = fuse::backend::generateDotCodeFrom(circuit);
                replaced_calls++;
                replaced = replaced + new_size;
                ctr++;
            }
        }
    }


    report << "\n"
           << "Circuit size after vec: " << circuit.getNumberOfNodes() << std::endl;
    report << "Replacement calls: " << replaced_calls << std::endl;
    report << "Replaced nodes: " << replaced << "\n"
           << std::endl;
}

void vectorizeAllInstructions(core::CircuitObjectWrapper& circuit, int minGates, int maxDistance) {
    // prepare tmp folder
    namespace fs = std::filesystem;
    const std::string output_dir = "../../tmp/";
    if (!fs::exists(output_dir)) {
        fs::create_directory(output_dir);
    }
    std::string report_str = output_dir + "MIVreport.txt";
    std::ofstream report(report_str, std::ios::trunc);

    for (auto instructionType = static_cast<int>(core::ir::PrimitiveOperation::MIN); instructionType != static_cast<int>(core::ir::PrimitiveOperation::MAX); instructionType++) {
        auto cur_type = static_cast<core::ir::PrimitiveOperation>(instructionType);
        if (cur_type != core::ir::PrimitiveOperation::Input && cur_type != core::ir::PrimitiveOperation::Output) {
            vectorizeInstructions(circuit, cur_type, minGates, maxDistance, true);
        }
    }
}

}  // namespace fuse::passes
