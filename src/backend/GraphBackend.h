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

#ifndef FUSE_GRAPHBACKEND_H
#define FUSE_GRAPHBACKEND_H

#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include "BaseVisitor.h"
#include "ModuleWrapper.h"

#include <boost/multiprecision/cpp_int.hpp>

namespace fuse::backend {

std::string generateDistgraphFrom(core::CircuitObjectWrapper& circuit);

std::string generateGlasgowgraphFrom(core::CircuitObjectWrapper& circuit);

std::string translateDistgraphToGlasgow(std::string DistgraphPattern);

std::vector<uint64_t> translateMappingToNodevec(std::string DistgraphMapping);

boost::multiprecision::cpp_int glasglowSubgraphFinding(std::string output_dir, int ctr, std::string graphFile, std::string patternFile);

boost::multiprecision::cpp_int glasglowSubgraphCounting(std::string output_dir, int ctr, std::string graphFile, std::string patternFile);

std::string frequentSubgraphMining(std::string output_dir, int ctr, std::string filename, int frequency_threshold);

int postProcessDistgraph(const std::string output_filename, int mode);

int postProcessGlasgow(std::set<uint64_t>& already_replaced, std::string output_dir, const std::string output_filename, core::CircuitObjectWrapper &circuit, boost::multiprecision::cpp_int count_glasgow, std::map<uint64_t, std::vector<uint64_t>> subgraph_input, std::unordered_map<uint64_t, std::unordered_set<uint64_t>> nodeSuccessors, std::unordered_map<uint64_t, uint64_t> nodeDepth);

std::vector<uint64_t> mapping_is_appliceable(std::set<uint64_t> already_replaced, std::string mapping);

bool mapping_is_compatible(fuse::core::CircuitObjectWrapper &circuit, std::vector<uint64_t> nodes_to_replace, std::map<uint64_t, std::vector<uint64_t>> subgraph_input);

bool recursive_mapping_legal(uint64_t cur, int maxDepth, std::unordered_map<uint64_t, std::unordered_set<uint64_t>> &nodeSuccessors, std::vector<uint64_t> &nodes_to_replace, std::unordered_map<uint64_t, uint64_t> &nodeDepth);

bool mapping_is_legal(std::unordered_map<uint64_t, std::unordered_set<uint64_t>> &nodeSuccessors, std::vector<uint64_t> &nodes_to_replace, std::unordered_map<uint64_t, uint64_t> &nodeDepth);

}  // namespace fuse::backend

#endif /* FUSE_GRAPHBACKEND_H */
