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

#include "FrequentSubcircuitReplacement.h"

#include <stdio.h>
#include <sys/wait.h>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <signal.h>
#include <vector>

#include "DOTBackend.h"
#include "GraphBackend.h"
#include "ModuleGenerator.h"
#include "NodeSuccessorsAnalysis.h"
#include "DepthAnalysis.h"

namespace fuse::passes {

uint64_t recursively_create_node(core::CircuitObjectWrapper &circuit, std::vector<uint64_t> &nodes_to_replace, uint64_t cur_node_id, frontend::CircuitBuilder &cb, std::map<uint64_t, fuse::frontend::Identifier> &created_gates, std::map<uint64_t, std::vector<uint64_t>> &subgraph_input, std::map<uint64_t, bool> &is_input) {
    // recursive anchor 1: already created (and all its predecessors)
    // dynamic programming
    if (!is_input[cur_node_id] && created_gates.find(cur_node_id) != created_gates.end()) {
        return created_gates[cur_node_id];
    }

    // recursive anchor 2: if not part of nodes in circuit, create input node
    auto node_iterator = std::find(nodes_to_replace.begin(), nodes_to_replace.end(), cur_node_id);
    if (node_iterator == nodes_to_replace.end()) {
        auto dummy_type = cb.addDataType(core::ir::PrimitiveType::Bool, core::ir::SecurityLevel::Plaintext);
        auto in_gate = cb.addInputNode(dummy_type);
        is_input[in_gate] = true;
        return in_gate;
    }

    // recursion: call function for every predecessor
    auto node = circuit.getNodeWithID(cur_node_id);
    int node_index = std::distance(nodes_to_replace.begin(), node_iterator);
    std::vector<uint64_t> predVec;

    for (auto pred_id : node.getInputNodeIDs()) {
        // get predecessors
        int node_id = recursively_create_node(circuit, nodes_to_replace, pred_id, cb, created_gates, subgraph_input, is_input);

        predVec.push_back(node_id);
        if (is_input[node_id]) {
            subgraph_input[node_index].push_back(node_id);
        }
    }

    // recursive combination of predecessors
    auto gate = cb.addNode(node.getOperation(), predVec);
    is_input[created_gates[cur_node_id]] = false;
    created_gates[cur_node_id] = gate;
    return gate;
}

std::unordered_map<uint64_t, std::vector<uint64_t>> get_output_mapping(std::vector<uint64_t> &nodes_to_replace, std::map<uint64_t, fuse::frontend::Identifier> &subgraph_output, std::unordered_map<uint64_t, std::unordered_set<uint64_t>> nodeSuccessors) {
    std::unordered_map<uint64_t, std::vector<uint64_t>> output_mapping;

    std::vector<uint64_t>::iterator it;
    for (it = nodes_to_replace.begin(); it != nodes_to_replace.end(); ++it) {
        // get index
        int index = std::distance(nodes_to_replace.begin(), it);

        // add to mapping
        auto out_gate = subgraph_output[index];
        for (auto suc_id : nodeSuccessors[*it]) {
            auto node_iterator = std::find(nodes_to_replace.begin(), nodes_to_replace.end(), suc_id);
            // for every node outside of pattern, add output
            if (node_iterator == nodes_to_replace.end()) {
                output_mapping[out_gate].push_back(suc_id);
            }
        }
    }
    return output_mapping;
}

std::unordered_map<uint64_t, uint64_t> get_input_mapping(core::CircuitObjectWrapper &circuit, std::vector<uint64_t> &nodes_to_replace, std::map<uint64_t, std::vector<uint64_t>> &subgraph_input) {
    std::unordered_map<uint64_t, uint64_t> input_mapping;

    std::vector<uint64_t>::iterator it;
    for (it = nodes_to_replace.begin(); it != nodes_to_replace.end(); ++it) {
        // get index
        int index = std::distance(nodes_to_replace.begin(), it);

        // check if node has inputs
        if (subgraph_input.find(index) != subgraph_input.end()) {
            // add to mapping
            auto input_vec = subgraph_input[index];
            auto node = circuit.getNodeWithID(*it);
            int index = 0;
            for (auto pred_id : node.getInputNodeIDs()) {
                auto node_iterator = std::find(nodes_to_replace.begin(), nodes_to_replace.end(), pred_id);
                if (node_iterator == nodes_to_replace.end()) {
                    // andersrum
                    input_mapping[input_vec[index]] = pred_id;
                    index++;
                }
            }
        }
    }
    return input_mapping;
}

core::CircuitContext create_circuit_to_call(core::CircuitObjectWrapper &circuit, std::vector<uint64_t> &nodes_to_replace, std::unordered_map<uint64_t, std::unordered_set<uint64_t>> nodeSuccessors, std::map<uint64_t, std::vector<uint64_t>> &subgraph_input, std::map<uint64_t, fuse::frontend::Identifier> &subgraph_output) {
    // init circuit builder
    frontend::CircuitBuilder cb(std::to_string(nodes_to_replace[0]));

    // construct circuit by iterating over every node
    std::map<uint64_t, fuse::frontend::Identifier> created_gates;
    std::map<uint64_t, bool> is_input;
    for (auto node_id : nodes_to_replace) {
        recursively_create_node(circuit, nodes_to_replace, node_id, cb, created_gates, subgraph_input, is_input);
    }

    // add output nodes in circuit
    for (auto node_id : nodes_to_replace) {
        auto node_it = std::find(nodes_to_replace.begin(), nodes_to_replace.end(), node_id);

        int node_index = std::distance(nodes_to_replace.begin(), node_it);

        // if not already created, create output gate
        if (subgraph_output.find(node_index) == subgraph_output.end()) {
            auto dummy_type = cb.addDataType(core::ir::PrimitiveType::Bool, core::ir::SecurityLevel::Plaintext);
            auto out_gate = cb.addOutputNode(dummy_type, {created_gates[node_id]});

            subgraph_output[node_index] = out_gate;
        }
    }

    // finish circuit
    cb.finish();

    // return circuit context
    core::CircuitContext context(cb);
    auto name_x = context.getReadOnlyCircuit()->getName();
    return context;
}

std::vector<uint64_t> find_first_valid_embedding(std::set<uint64_t> already_replaced, const std::string output_filename, std::unordered_map<uint64_t, std::unordered_set<uint64_t>> &nodeSuccessors, std::unordered_map<uint64_t, uint64_t> &nodeDepth) {

    // create input file
    std::ifstream inf;
    inf.open(output_filename, std::ios::in);

    std::string cur_mapping;
    std::vector<uint64_t> nodes_to_replace;
    while(inf){
        // get new line
        std::getline(inf, cur_mapping);

        // terminate
        if (cur_mapping.empty()) {
            break;
        }

        nodes_to_replace = fuse::backend::mapping_is_appliceable(already_replaced, cur_mapping);

        if (!nodes_to_replace.empty()){

            if (fuse::backend::mapping_is_legal(nodeSuccessors, nodes_to_replace, nodeDepth)){
                return nodes_to_replace;
            }
        }
    }
    return std::vector<uint64_t>();
}

fuse::core::ModuleContext replaceFrequentSubcircuits(core::CircuitContext &circuitContext, int frequency_threshold, int mode, std::string distgraphpath, std::string glasgowpath) {
    namespace fs = std::filesystem;
    const std::string output_dir = "../../tmp/";
    if (!fs::exists(output_dir)) {
        fs::create_directory(output_dir);
    }

    int replaced = 0;
    int replaced_calls = 0;
    int ctr = 0;

    // prepare module builder
    core::CircuitObjectWrapper circuit = circuitContext.getMutableCircuitWrapper();
    frontend::ModuleBuilder mb;

    // check which tmp files already exist
    std::string distgraph = output_dir + "distgraph" + std::to_string(ctr) + ".txt";
    while (fs::exists(distgraph)) {
        ctr++;
        distgraph = output_dir + "distgraph" + std::to_string(ctr) + ".txt";
    }

    // create report
    std::string report_str = output_dir + "FSRreport" + std::to_string(ctr) + ".txt";
    std::ofstream report(report_str, std::ios::trunc);
    report << "Circuit size before replacement: " << circuit.getNumberOfNodes() << "\n"
           << std::endl;
    report.flush();

    // 1. apply graph translation and write to graph file (Distgraph Graph)
    std::string output_filename;
    int count_patterns;
    if (distgraphpath.empty()){
        std::ofstream of(distgraph, std::ios::trunc);
        of << fuse::backend::generateDistgraphFrom(circuit);
        of.flush();
        of.close();
    }
    else{
        distgraph = distgraphpath;
    }

    // 2. apply Frequent SubgraphMining and post process
    output_filename = fuse::backend::frequentSubgraphMining(output_dir, ctr, distgraph, frequency_threshold);
    count_patterns = fuse::backend::postProcessDistgraph(output_filename, mode);

    // 3. apply graph translation and write to graph file (Glasgow Graph)
    std::string glasgow_graph;
    if (glasgowpath.empty()){
        glasgow_graph = output_dir + "glasgowgraph" + std::to_string(ctr) + ".csv";
        std::ofstream of(glasgow_graph, std::ios::trunc);
        of << fuse::backend::generateGlasgowgraphFrom(circuit);
        of.flush();
        of.close();
    }
    else{
        glasgow_graph = glasgowpath;
    }

    // 4. apply Glasgow Subgraph Solver for each frequent pattern and replace

    // read frequent subgraphs one by one
    std::ifstream inf(output_filename + "_opt", std::ios::in);
    std::string cur_line;
    int pattern_ctr = 1;
    std::set<uint64_t> already_replaced;

    while (std::getline(inf, cur_line)) {

        // read Distgraph Pattern and translate
        if (cur_line.empty()) {
            break;
        }
        std::string pattern_file = output_dir + "pattern" + std::to_string(ctr) + ".csv";
        std::ofstream of(pattern_file, std::ios::trunc);
        of << fuse::backend::translateDistgraphToGlasgow(cur_line);
        of.flush();
        of.close();

        // apply Glasgow Solver for pattern
        boost::multiprecision::cpp_int count_glasgow = fuse::backend::glasglowSubgraphFinding(output_dir, ctr, glasgow_graph, pattern_file);

        // apply node successor analysis and get node depths
        std::unordered_map<uint64_t, std::unordered_set<uint64_t>> nodeSuccessors = fuse::passes::getNodeSuccessors(circuit);
        std::unordered_map<uint64_t, uint64_t> nodeDepth = fuse::passes::getNodeDepths(circuit);

        // create subgraph
        output_filename = output_dir + "mappings" + std::to_string(ctr);
        std::map<uint64_t, std::vector<uint64_t>> subgraph_input;
        std::map<uint64_t, fuse::frontend::Identifier> subgraph_output;
        std::map<uint64_t, bool> is_input;
        std::vector<uint64_t> first_embedding = find_first_valid_embedding(already_replaced, output_filename, nodeSuccessors, nodeDepth);

        // optimization: no applicable embedding
        if (first_embedding.empty()){
            // update report
            report << "Not a single valid embedding for pattern " << pattern_ctr << "/" << count_patterns << std::endl << std::endl;;
            break;
        }

        core::CircuitContext subcircuit = create_circuit_to_call(circuit, first_embedding, nodeSuccessors, subgraph_input, subgraph_output);
        auto subcircuitReadOnly = subcircuit.getReadOnlyCircuit();
        mb.addSerializedCircuit(subcircuit.getBufferPointer(), subcircuit.getBufferSize());

        // update report
        report << "Pattern: \n";
        report << fuse::backend::generateDotCodeFrom(*subcircuitReadOnly);

        // filter embeddings
        // check embeding for unallowed dependencies (subcircuit is dependend on its own output)
        report << "Filtering " << count_glasgow << " embeddings" << std::endl;
        std::string filterreportpath = output_dir + "Filterreport.txt";
        report << "Details in " << filterreportpath << std::endl;
        int count_embeddings = fuse::backend::postProcessGlasgow(already_replaced, output_dir, output_filename, circuit, count_glasgow, subgraph_input, nodeSuccessors, nodeDepth);
        report << "Filtered embeddings " << count_embeddings << "/" << count_glasgow << std::endl;

        // read filtered output
        output_filename = output_dir + "mappings" + std::to_string(ctr) + "_opt";
        std::ifstream inf_mapping(output_filename, std::ios::in);

        std::string cur_mapping;
        std::vector<uint64_t> nodes_to_replace;
        int individ_replaced_calls = 0;
        int individ_replaced = 0;
        int emb_ctr = 1;

        report << "\n" << "Embedding-STATUS for pattern " << pattern_ctr << "/" << count_patterns << std::endl;

        while (std::getline(inf_mapping, cur_mapping)) {

            // status logging
            if (emb_ctr % 100 == 1){
                report << "Embedding " << emb_ctr << "/" << count_embeddings << std::endl;
                report << individ_replaced << " Nodes Replaced" << "\n" << std::endl;
            }

            // terminate
            if (cur_mapping.empty()) {
                break;
            }

            // get nodes
            nodes_to_replace = fuse::backend::translateMappingToNodevec(cur_mapping);

            // apply node successor analysis
            nodeSuccessors = fuse::passes::getNodeSuccessors(circuit);

            // check again of legality of pattern has changed (otherwise skip)
            bool legal = fuse::backend::mapping_is_legal(nodeSuccessors, nodes_to_replace, nodeDepth);
            if (!legal){
                continue;
            }

            // get input for output mapping for concrete embedding
            auto input_mapping = get_input_mapping(circuit, nodes_to_replace, subgraph_input);
            auto output_mapping = get_output_mapping(nodes_to_replace, subgraph_output, nodeSuccessors);

            // translate subgraph output
            std::unordered_map<uint64_t, fuse::frontend::Identifier> translated_subgraph_output;

            // translate subgraph output
            for (auto pair : subgraph_output) {
                translated_subgraph_output[pair.second] = nodes_to_replace[pair.first];
            }

            uint64_t newid = circuit.replaceNodesBySubcircuit(*subcircuitReadOnly, nodes_to_replace, input_mapping, output_mapping, translated_subgraph_output);

            // update nodedepth for newly created node
            int minDepth = nodeDepth[nodes_to_replace[0]];
            for (auto node : nodes_to_replace) {
                minDepth = nodeDepth[node] < minDepth ? nodeDepth[node] : minDepth;
            }
            nodeDepth[newid] = minDepth;

            individ_replaced_calls++;
            replaced_calls++;
            replaced = replaced + nodes_to_replace.size();
            individ_replaced = individ_replaced + nodes_to_replace.size();
            emb_ctr++;
        }

        // update report
        report << "Replacement calls: " << individ_replaced_calls << std::endl;
        report << "Replaced nodes: " << individ_replaced << "\n"
                << std::endl;
        pattern_ctr++;
    }

    // finalize report
    report << "Circuit size after replacement: " << circuit.getNumberOfNodes() << std::endl;
    report << "Total replacement calls: " << replaced_calls << std::endl;
    report << "Total replaced nodes: " << replaced << std::endl;

    circuitContext.packCircuit();
    mb.addSerializedCircuit(circuitContext.getBufferPointer(), circuitContext.getBufferSize());
    mb.setEntryCircuitName(circuitContext.getReadOnlyCircuit()->getName());
    fuse::core::ModuleContext context(mb);
    return context;
}

int child_pid = 0;
void timer_handler(int pid) {
    int res = kill(child_pid, SIGKILL);
}

int countLines(std::string filename) {
    std::ifstream inf;
    inf.open(filename, std::ios::in);
    int lines = 0;

    std::string cur_line;
    while (inf) {
        // get new line
        std::getline(inf, cur_line);

        // add line
        lines++;
    }

    return lines;
}

fuse::core::ModuleContext automaticallyReplaceFrequentSubcircuits(fuse::core::CircuitContext &circuitContext, int try_modes, int timeoutseconds, int pattern_upper, int pattern_lower) {
    // create report
    namespace fs = std::filesystem;
    const std::string output_dir = "../../tmp/";
    if (!fs::exists(output_dir)) {
        fs::create_directory(output_dir);
    }
    std::string report_str = output_dir + "AFSRreport.txt";
    std::ofstream report(report_str, std::ios::trunc);

    // check which tmp files already exist
    int ctr = 0;
    std::string distgraph = output_dir + "distgraph" + std::to_string(ctr) + ".txt";
    while (fs::exists(distgraph)) {
        ctr++;
        distgraph = output_dir + "distgraph" + std::to_string(ctr) + ".txt";
    }

    // translate circuit to DistGraph
    std::ofstream of(distgraph, std::ios::trunc);
    core::CircuitObjectWrapper circuit = circuitContext.getMutableCircuitWrapper();
    of << fuse::backend::generateDistgraphFrom(circuit);
    of.flush();
    of.close();

    // 1. find good frequency parameter
    int cur_frequency = circuitContext.getReadOnlyCircuit()->getNumberOfNodes() / 2;
    int step_size = cur_frequency;
    signal(SIGALRM, timer_handler);

    // step 1.1: binary search for begin of meaningful patterns with short timeout
    report << "Binary Search:"
           << std::endl;
    int iterations = 0;
    int max_iterations = std::log2(circuitContext.getReadOnlyCircuit()->getNumberOfNodes());
    std::set<int> timedout;
    std::set<int> worked;

    while (true) {
        // edge case
        if (cur_frequency < 2) {
            cur_frequency = 2;
            break;
        }
        // timedout case
        if (timedout.contains(cur_frequency)) {
            cur_frequency += step_size;
            break;
        }

        // worked case
        if (worked.contains(cur_frequency)) {
            break;
        }

        // update parameters
        step_size = (iterations < max_iterations - 1) ? step_size / 2 : step_size;
        iterations++;

        // calculate name before fork
        std::string output_filename = output_dir + "out" + std::to_string(ctr);

        report << "Trying frequency: " << cur_frequency << std::endl;
        report.flush();

        // fork child process that can be terminated after timeout
        pid_t pid = fork();

        if (pid == 0) {  // child
            fuse::backend::frequentSubgraphMining(output_dir, ctr, distgraph, cur_frequency);
            exit(EXIT_SUCCESS);
        } else {  // parent
            // check for timeout
            sleep(1); // to prevent instant killing
            int state;
            child_pid = pid;
            alarm(timeoutseconds);
            waitpid(pid, &state, 0);
            // timeout
            if (!WIFEXITED(state)) {
                report << "Timeout or Killed" << std::endl;
                timedout.insert(cur_frequency);
                cur_frequency += step_size;
            } else {  // no timeout
                worked.insert(cur_frequency);
                int line_count = countLines(output_filename);
                report << "Patterns identified: " << line_count << std::endl;

                if (iterations > max_iterations) {
                    break;
                } else if (line_count > pattern_upper){
                    cur_frequency += step_size;
                } else {
                    cur_frequency -= step_size;
                }
            }
        }
    }

    report << "Found good frequency threshold to begin incremental search at: " << cur_frequency << "\n\n";
    report.flush();

    // step 2: continue with incremental mode
    report << "Incremental Search:"
           << std::endl;

    // prepare lists
    timedout.clear();
    worked.clear();

    // Glasgow preperation
    std::string glasgow_graph = output_dir + "glasgowgraph" + std::to_string(ctr) + ".csv";
    of.open(glasgow_graph, std::ios::trunc);
    of << fuse::backend::generateGlasgowgraphFrom(circuit);
    of.flush();
    of.close();

    while (true) {
        // edge case
        if (cur_frequency < 2) {
            cur_frequency++;
            continue;
        }
        // timedout case
        if (timedout.contains(cur_frequency)) {
            cur_frequency++;
            continue;
        }
        // worked case
        if (worked.contains(cur_frequency)) {
            break;
        }

        report << "Trying frequency: " << cur_frequency << std::endl;
        report.flush();

        // calculate name before fork
        std::string output_filename = output_dir + "out" + std::to_string(ctr);

        // fork child process that can be terminated after timeout
        pid_t pid = fork();

        if (pid == 0) {  // child
            fuse::backend::frequentSubgraphMining(output_dir, ctr, distgraph, cur_frequency);
            exit(EXIT_SUCCESS);
        } else {  // parent

            // check for timeout
            sleep(1); // to prevent instant killing
            int state;
            child_pid = pid;
            waitpid(pid, &state, 0);
            std::cout << state << " - state" << std::endl;

            if (!WIFEXITED(state)) {  // timeout
                timedout.insert(cur_frequency);
                cur_frequency += 1;
                report << "Killed" << std::endl;
            } else {  // no timeout

                worked.insert(cur_frequency);

                int line_count = countLines(output_filename);
                report << "Patterns identified: " << line_count << std::endl;

                fuse::backend::postProcessDistgraph(output_filename, 0);
                std::ifstream inf(output_filename + "_opt", std::ios::in);
                std::string cur_line;
                boost::multiprecision::cpp_int max_counts = 0;

                // count embeddings (for first pattern)
                while (std::getline(inf, cur_line)) {
                    // read Distgraph Pattern and translate
                    if (cur_line.empty()) {
                        break;
                    }
                    std::string pattern_file = output_dir + "pattern" + std::to_string(ctr) + ".csv";
                    of.open(pattern_file, std::ios::trunc);
                    of << fuse::backend::translateDistgraphToGlasgow(cur_line);
                    of.flush();
                    of.close();

                    // apply glasgow to count embeddings
                    boost::multiprecision::cpp_int counts = fuse::backend::glasglowSubgraphCounting(output_dir, ctr, glasgow_graph, pattern_file);
                    max_counts = (max_counts > counts) ? max_counts : counts;
                }
                report << "Embeddings identified: " << max_counts << std::endl;

                // check if amount of patterns is sufficient
                if (line_count > pattern_upper) {
                    cur_frequency += 1;
                } else if (line_count < pattern_lower) {
                    cur_frequency -= 1;
                } else {
                    break;
                }
            }
        }
    }

    report << "Found good frequency threshold: " << cur_frequency << "\n\n";
    report.flush();

    // 2. try out different modes, to find the best option
    int initial_num = circuitContext.getReadOnlyCircuit()->getNumberOfNodes();
    int best_num = circuitContext.getReadOnlyCircuit()->getNumberOfNodes();
    int best_mode = -1;
    fuse::core::ModuleContext best_module;

    for (int mode = 0; mode < try_modes; mode++) {

        // find next counter
        int nex_ctr = 0;
        std::string test_str = output_dir + "distgraph" + std::to_string(nex_ctr) + ".txt";
        while (fs::exists(test_str)) {
            nex_ctr++;
            test_str = output_dir + "distgraph" + std::to_string(nex_ctr) + ".txt";
        }

        report << "Trying FSR with mode: " << mode << std::endl;
        report_str = output_dir + "FSRreport" + std::to_string(nex_ctr) + ".txt";
        report << "Status in file: " << report_str << std::endl;
        auto circuitContextCopy = circuitContext.createCopy();

        auto module = replaceFrequentSubcircuits(circuitContextCopy, cur_frequency, mode, distgraph, glasgow_graph);
        auto num_nodes = module.getReadOnlyModule()->getEntryCircuit()->getNumberOfNodes();

        // update currently best mode
        report << "Number of remaining nodes: " << num_nodes << "/" << initial_num << std::endl;
        if (num_nodes <= best_num) {
            best_num = num_nodes;
            best_mode = mode;
            best_module = std::move(module);
        }
    }
    report << "Found good mode: " << best_mode << std::endl;
    report.flush();
    return best_module;
}

}  // namespace fuse::passes
