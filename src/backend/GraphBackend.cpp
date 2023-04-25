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

#include "DepthAnalysis.h"
#include "NodeSuccessorsAnalysis.h"

#include "GraphBackend.h"
#include "list"
#include "set"

// Distgraph
#include <graph_types.hpp>
#include <dbio.hpp>
#include <graph_output.hpp>
#include <sys/time.h>
#include <mpi.h>
#include <graph_miner_mpi_split_all.hpp>
#include <utils.hpp>

// Glasgow
#include "formats/read_file_format.hh"
#include "homomorphism.hh"
#include "sip_decomposer.hh"
#include "lackey.hh"
#include "symmetries.hh"
#include "restarts.hh"
#include "verify.hh"
#include "proof.hh"
#include <boost/program_options.hpp>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>
#include <unistd.h>

namespace fuse::backend{

std::string generateDistgraphFrom(fuse::core::CircuitObjectWrapper& circuit) {
    
    // create the header string and the edges string for DistGraph
    std::string header = "t # 0\n";
    std::stringstream edges;

    // list to keep track of registered nodes 
    std::map<uint64_t, std::string> nodesMap;

    // iterate over all nodes
    circuit.topologicalTraversal([&](const fuse::core::NodeReadOnly& node) {
            
        if (!node.isInputNode() && !node.isOutputNode()){
            auto nodeID = node.getNodeID();

            // create vertex: 'v node_id node_label'
            std::stringstream nodes;
            nodes << "v ";
            nodes << nodeID;
            nodes << " ";
            nodes << static_cast<int>(node.getOperation());
            nodes << "\n";

            nodesMap[nodeID] = nodes.str();

            // create edges: 'e node_id1 node_id2 edge_label'
            // in our case, edge_label is always 0
            auto inputNodes = node.getInputNodeIDs();
            for (auto pred: inputNodes) {

                // create edges: 'e node_id1 node_id2 edge_label'
                // in our case, edge_label is always 0    
                auto cur_node = circuit.getNodeWithID(pred);
                if (!cur_node.isInputNode()){      
                    edges << "e ";
                    edges << pred;
                    edges << " ";
                    edges << nodeID;
                    edges << " 0\n";
                }
            }
        }
    });

    std::stringstream nodes;

    // build node string
    for(auto nodePair: nodesMap){
        nodes << nodePair.second;
    }

    // return the concatenation of the header string, the nodes string, and the edges string
    return header + nodes.str() + edges.str();
}

std::string generateGlasgowgraphFrom(fuse::core::CircuitObjectWrapper& circuit) {
    
    // create the edges string for Glasgow Subgraph Solver
    std::stringstream edges;

    // list to keep track of registered nodes 
    std::map<uint64_t, std::string> nodesMap;

    // iterate over all nodes
    std::unordered_map<uint64_t, std::unordered_set<uint64_t>> result;
    circuit.topologicalTraversal([&](const fuse::core::NodeReadOnly& node) {

        if (!node.isInputNode() && !node.isOutputNode()){
            auto nodeID = node.getNodeID();

            // create vertex: 'v node_id node_label'
            std::stringstream nodes;
            nodes << nodeID;
            nodes << ",,";
            nodes << static_cast<int>(node.getOperation());
            nodes << "\n";

            nodesMap[nodeID] = nodes.str();

            auto inputNodes = node.getInputNodeIDs();
            for (auto pred: inputNodes) {

                // create edge   
                auto cur_node = circuit.getNodeWithID(pred);
                if (!cur_node.isInputNode()){            
                    edges << pred;
                    edges << ",";
                    edges << nodeID;
                    edges << "\n";
                }
            }
        }
    });

    std::stringstream nodes;

    // build node string
    for(auto nodePair: nodesMap){
        nodes << nodePair.second;
    }

    // return the concatenation of the nodes string, and the edges string
    return edges.str() + nodes.str();
}

std::string translateDistgraphToGlasgow(std::string DistgraphPattern){

    // create the edges string for Glasgow Subgraph Solver
    std::stringstream edges;

    // list to keep track of registered nodes 
    std::map<uint64_t, std::string> nodesMap;
    
    //split string 
    char *token = strtok((char *)DistgraphPattern.c_str(), ";");
    std::list<std::string> entries;
    
    // Keep processing tokens while one of the
    // delimiters present in str[].
    while (token != NULL){
            std::string entry(token);

            // remove brackets 
            entry = entry.substr(1);

            // remove brackets 
            entry = entry.substr(0, entry.find(')'));

            // add entry 
            entries.push_back(entry);

            // next token
            token = strtok(NULL, ";");
    }

    for(std::string entry: entries){
        // split string (process 5 entries: vertex1_id vertix2_id vertex1_label 0 vertex2_label)
        char *internal_token = strtok((char *)entry.c_str(), " ");

        std::vector<char*> entrylst;
        while (internal_token != NULL){

            entrylst.push_back(internal_token);

            // next internal token
            internal_token = strtok(NULL, " ");
        }

        // create an edge
        edges << entrylst.at(0);
        edges << ",";
        edges << entrylst.at(1);
        edges << "\n";

        // add nodes if not added previously
        if (nodesMap.find(std::atoi(entrylst.at(0))) == nodesMap.end()){

            std::stringstream nodestring;

            nodestring << entrylst.at(0);
            nodestring << ",,";
            nodestring << entrylst.at(2);
            nodestring << "\n";

            nodesMap[std::atoi(entrylst.at(0))] = nodestring.str();
        }

        if (nodesMap.find(std::atoi(entrylst.at(1))) == nodesMap.end()){

            std::stringstream nodestring;

            nodestring << entrylst.at(1);
            nodestring << ",,";
            nodestring << entrylst.at(4);
            nodestring << "\n";

            nodesMap[std::atoi(entrylst.at(1))] = nodestring.str();
        }
    }

    std::stringstream nodes;

    // build node string
    for(auto nodePair: nodesMap){
        nodes << nodePair.second;
    }

    // return the concatenation of the nodes string, and the edges string
    return edges.str() + nodes.str();
}

std::vector<uint64_t> translateMappingToNodevec(std::string DistgraphMapping){

    std::vector<uint64_t> nodes;

     // remove first and last bracket (and spacing)
     DistgraphMapping = DistgraphMapping.substr(1);
     DistgraphMapping.pop_back();
     DistgraphMapping.pop_back();
    
    //split string 
    char *token = strtok((char *)DistgraphMapping.c_str(), ") (");
    std::list<std::string> entries;
    int ctr = 0;
    
    // Keep processing tokens while one of the
    // delimiters present in str[].
    while (token != NULL){
            std::string entry(token);
            
            // add every third node
            if (ctr == 2){
                nodes.push_back(atoi(entry.c_str()));
            } 

            // next token
            ctr = (ctr + 1) % 3;
            token = strtok(NULL, ") (");
    }

    return nodes;
}

int postProcessDistgraph(const std::string output_filename, int mode){

    // create input file
    std::ifstream inf;
    inf.open(output_filename, std::ios::in);

    // create output file
    std::ofstream of;
    of.open(output_filename + "_opt", std::ios::out);

    // sort patterns by size and filter by gate_threshold
    std::map<int, std::list<std::string>> sorted_patterns;
    std::string cur_line;
    while(inf){
        // get new line 
        std::getline(inf, cur_line);

        // count edges-1 in prev_line
        size_t n = std::count(cur_line.begin(), cur_line.end(), ';');

        sorted_patterns[n].push_back(cur_line);
    }

    // transfer correct patterns in new file 
    for (auto iter = sorted_patterns.rbegin(); iter != sorted_patterns.rend(); ++iter) {
        // differnt modes supported (0 for largest, 1 for second largest etc.)
        if (iter->second.size()>0 && mode==0){
            for (auto lst_entry: iter->second){
                of << lst_entry << endl;
            }
            of.flush();
            return iter->second.size();
        }
        else if (iter->second.size()>0){
            mode--;
        }
    }
    return 0;
}

int postProcessGlasgow(std::set<uint64_t>& already_replaced, std::string output_dir, const std::string output_filename, fuse::core::CircuitObjectWrapper &circuit, boost::multiprecision::cpp_int count_glasgow, std::map<uint64_t, std::vector<uint64_t>> subgraph_input, std::unordered_map<uint64_t, std::unordered_set<uint64_t>> nodeSuccessors, std::unordered_map<uint64_t, uint64_t> nodeDepth){


    std::string report_str = output_dir + "Filterreport.txt";
    std::ofstream report(report_str, std::ios::trunc);

    // create input file
    std::ifstream inf;
    inf.open(output_filename, std::ios::in);

    // create output file
    std::ofstream of;
    of.open(output_filename + "_opt", std::ios::out);

    std::string cur_mapping;
    int ptrns = 0;
    int iter = 0;
    std::vector<uint64_t> nodes_to_replace;
    while(inf){
        // get new line 
        std::getline(inf, cur_mapping);

        // terminate 
        if (cur_mapping.empty()) {
            return ptrns;
        }else if (iter % 10000 == 0){
            report << "Embedding " << iter << "/" << count_glasgow << std::endl;
            report << ptrns << " embeddings kept so far" << "\n" << std::endl;
        }

        // check if nodes still available for replacement 
        nodes_to_replace = mapping_is_appliceable(already_replaced, cur_mapping);

        if (!nodes_to_replace.empty()){

            // check if embedding is compatible to pattern
            bool compatible = mapping_is_compatible(circuit, nodes_to_replace, subgraph_input);

            if (compatible){

                // check if nodes have recusive dependencies (output feeds into input)
                bool legal = mapping_is_legal(nodeSuccessors, nodes_to_replace, nodeDepth);

                // insert replaced nodes
                if (legal){
                    of << cur_mapping << endl;
                    ptrns++;
                    for(auto node : nodes_to_replace)
                        already_replaced.insert(node);
                }
            }
        }
        iter++;
    }
    return ptrns;
}

bool mapping_is_compatible(fuse::core::CircuitObjectWrapper &circuit, std::vector<uint64_t> nodes_to_replace, std::map<uint64_t, std::vector<uint64_t>> subgraph_input){
    
    std::vector<uint64_t>::iterator it;
    for (it = nodes_to_replace.begin(); it != nodes_to_replace.end(); ++it) {
        int ctr = 0;
        
        // get index
        int index = std::distance(nodes_to_replace.begin(), it);

        // check if node has inputs
        if (subgraph_input.find(index) != subgraph_input.end()) {
            auto input_vec = subgraph_input[index];
            auto node = circuit.getNodeWithID(*it);

            int ctr = 0;
            for (auto pred_id : node.getInputNodeIDs()) {
                auto node_iterator = std::find(nodes_to_replace.begin(), nodes_to_replace.end(), pred_id);
                if (node_iterator == nodes_to_replace.end()) {
                    ctr++;

                    // embedding has more inputs for that node than is allowed by the subgraph
                    if (input_vec.size() < ctr){
                        return false;
                    }
                }
            }

            // embedding has too few inputs for that node than is allowed by the subgraph
            if (input_vec.size() > ctr){
                return false;
            } 
        }
    }
    return true;
}

std::vector<uint64_t> mapping_is_appliceable(std::set<uint64_t> already_replaced, std::string mapping) {
    
    std::vector<uint64_t> replaceable_nodes = fuse::backend::translateMappingToNodevec(mapping);

    for (auto node_id : replaceable_nodes) {
        if (already_replaced.contains(node_id)){
            std::vector<uint64_t> empty_vec;
            return empty_vec;
        }
    }

    return replaceable_nodes;
}

bool mapping_is_legal(std::unordered_map<uint64_t, std::unordered_set<uint64_t>> &nodeSuccessors, std::vector<uint64_t> &nodes_to_replace, std::unordered_map<uint64_t, uint64_t> &nodeDepth) {
    // get maximal depth of embedding
    int maxDepth = 0;
    for (auto node : nodes_to_replace) {
        maxDepth = nodeDepth[node] > maxDepth ? nodeDepth[node] : maxDepth;
    }

    // add outputs to working set 
    std::deque<uint64_t> working_set;
    std::set<uint64_t> already_processed;

    for (auto node : nodes_to_replace) {
        for (auto succ : nodeSuccessors[node]) {
            auto node_iterator = std::find(nodes_to_replace.begin(), nodes_to_replace.end(), succ);

            // if output node
            if (node_iterator == nodes_to_replace.end() && !already_processed.contains(succ)){
                working_set.push_back(succ);
                already_processed.insert(succ);
            }
        }
    }

    // process each element in the working set 
    while(!working_set.empty()){

        // get first element 
        uint64_t cur = *working_set.begin();
        working_set.pop_front();

        // depth truncation
        if (nodeDepth[cur] > maxDepth) {
            continue;
        }

        // circular dependency!
        auto it = std::find(nodes_to_replace.begin(), nodes_to_replace.end(), cur);
        if (it != nodes_to_replace.end()) {
            return false;
        }

        // further search
        for (auto succ : nodeSuccessors[cur]) {
            if (!already_processed.contains(succ)){
                working_set.push_back(succ);
                already_processed.insert(succ);
            }
        }
    }

    return true;
}

std::string frequentSubgraphMining(std::string output_dir, int ctr, std::string filename, int frequency_threshold){

    // Graph postprocessing
    std::string filetype = "txt";
    std::string output_filename = output_dir + "out" + std::to_string(ctr);

    types::Graph graph;

    dbio::FILE_TYPE file_format;
    file_format = dbio::ftype2str(filetype);

    dbio::read_graph(file_format, filename, graph);

    // apply Distgraph (sequential)
    graph_file_output *file_out = new graph_file_output(output_filename);
    graph_output *current_out = file_out;

    GRAPH_MINER::graph_miner fsm;

    fsm.set_graph(graph);
    fsm.set_min_support(frequency_threshold);
    fsm.set_graph_output(current_out);
    fsm.run();
    
    delete file_out;

    // sequential end 

    // return path of output 
    return output_filename;
}

boost::multiprecision::cpp_int glasglowSubgraphFinding(std::string output_dir, int ctr, std::string graphFile, std::string patternFile){

        /* Read in the graphs */
        auto pattern = read_file_format("auto", patternFile);
        auto target = read_file_format("auto", graphFile);

        HomomorphismParams params;
        
        params.injectivity = Injectivity::Injective;
        params.induced = 0;
        params.triggered_restarts = 0;
        params.restarts_schedule = std::make_unique<NoRestartsSchedule>();
        params.clique_detection = 0;
        params.distance3 = 0;
        params.k4 = 0;
        params.no_supplementals = 0;
        params.no_nds = 0;
        params.clique_size_constraints = 0;
        params.clique_size_constraints_on_supplementals = 0;
        params.send_partials_to_lackey = 0;
        params.propagate_using_lackey = PropagateUsingLackey::Never;
        params.timeout = std::make_shared<Timeout>(std::chrono::seconds{0});
        params.start_time = std::chrono::steady_clock::now();
        
        /* print all solutions */
        params.count_solutions = 1;

        std::string mappingsfile = output_dir + "mappings" + std::to_string(ctr);
        std::ofstream of;
        of.open(mappingsfile, std::ios::trunc);
        params.enumerate_callback = [&] (const VertexToVertexMapping & mapping) -> bool {
            for (auto v : mapping)
                of << "(" << pattern.vertex_name(v.first) << " -> " << target.vertex_name(v.second) << ") ";
            of << endl;
            return true;
            };
        params.start_time = std::chrono::steady_clock::now();

        // /* apply solver */
        auto result = solve_homomorphism_problem(pattern, target, params);
        
        of.flush();
        of.close();

        verify_homomorphism(pattern, target, params.injectivity == Injectivity::Injective, params.injectivity == Injectivity::LocallyInjective,
                params.induced, result.mapping);

        return result.solution_count;
}

boost::multiprecision::cpp_int glasglowSubgraphCounting(std::string output_dir, int ctr, std::string graphFile, std::string patternFile){

        /* Read in the graphs */
        auto pattern = read_file_format("auto", patternFile);
        auto target = read_file_format("auto", graphFile);

        HomomorphismParams params;
        
        params.injectivity = Injectivity::Injective;
        params.induced = 0;
        params.triggered_restarts = 0;
        params.restarts_schedule = std::make_unique<NoRestartsSchedule>();
        params.clique_detection = 0;
        params.distance3 = 0;
        params.k4 = 0;
        params.no_supplementals = 0;
        params.no_nds = 0;
        params.clique_size_constraints = 0;
        params.clique_size_constraints_on_supplementals = 0;
        params.send_partials_to_lackey = 0;
        params.propagate_using_lackey = PropagateUsingLackey::Never;
        params.timeout = std::make_shared<Timeout>(std::chrono::seconds{0});
        params.start_time = std::chrono::steady_clock::now();
        
        /* print all solutions */
        params.count_solutions = 1;

        std::string mappingsfile = output_dir + "mappings" + std::to_string(ctr);
        std::ofstream of;
        of.open(mappingsfile, std::ios::trunc);
        params.start_time = std::chrono::steady_clock::now();

        // /* apply solver */
        auto result = solve_homomorphism_problem(pattern, target, params);
        
        of.flush();
        of.close();

        verify_homomorphism(pattern, target, params.injectivity == Injectivity::Injective, params.injectivity == Injectivity::LocallyInjective,
                params.induced, result.mapping);

        return result.solution_count;
}

}