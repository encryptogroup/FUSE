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
#include <queue>
namespace fuse::passes {

std::unordered_map<uint64_t, uint64_t> getNodeDepths(const core::CircuitReadOnly& circuit){
    std::unordered_map<uint64_t, uint64_t> depth;

    // apply node successor analysis
    std::unordered_map<uint64_t, std::unordered_set<uint64_t>> nodeSuccessors = fuse::passes::getNodeSuccessors(circuit);  

    // bfs
    std::queue<uint64_t> bfs_queue;
    std::set<uint64_t> bfs_entries;
    for(auto node: circuit.getInputNodeIDs()){
        bfs_queue.push(node);
        bfs_entries.insert(node);
    }

    while(!bfs_queue.empty()){

        // pop first element 
        uint64_t cur_node = bfs_queue.front();
        bfs_queue.pop();
        bfs_entries.erase(cur_node);

        // calculate depth 
        uint64_t max_depth = 0;
        auto cur_n = circuit.getNodeWithID(cur_node);
        bool skip = false;
        for(auto pred: (*cur_n).getInputNodeIDs()){
            
            // skip if not all preds set (will be called later)
            if (!depth.contains(pred)){
                skip = true;
                break;
            }

            max_depth = max_depth > depth[pred] ? max_depth : depth[pred];
        }

        if (skip){
            continue;
        }

        depth[cur_node] = max_depth + 1;

        // add successors 
        for(auto successor: nodeSuccessors[cur_node]){
            if (!bfs_entries.contains(successor)){
                bfs_queue.push(successor);
                bfs_entries.insert(successor);
            }
        }
    }

    return depth;
}

std::unordered_map<uint64_t, uint64_t> getNodeInstructionDepths(const core::CircuitReadOnly& circuit, core::ir::PrimitiveOperation operationType){
    std::unordered_map<uint64_t, uint64_t> depth;

    // apply node successor analysis
    std::unordered_map<uint64_t, std::unordered_set<uint64_t>> nodeSuccessors = fuse::passes::getNodeSuccessors(circuit);  

    // bfs
    std::queue<uint64_t> bfs_queue;
    std::set<uint64_t> bfs_entries;
    for(auto node: circuit.getInputNodeIDs()){
        bfs_queue.push(node);
        bfs_entries.insert(node);
    }

    while(!bfs_queue.empty()){

        // pop first element 
        uint64_t cur_node = bfs_queue.front();
        bfs_queue.pop();
        bfs_entries.erase(cur_node);

        // calculate depth 
        uint64_t max_depth = 0;
        auto cur_n = circuit.getNodeWithID(cur_node);
        bool skip = false;
        for(auto pred: (*cur_n).getInputNodeIDs()){
            
            // skip if not all preds set (will be called later)
            if (!depth.contains(pred)){
                skip = true;
                break;
            }

            max_depth = max_depth > depth[pred] ? max_depth : depth[pred];
        }

        if (skip){
            continue;
        }

        if ((*cur_n).getOperation() == operationType){
            depth[cur_node] = max_depth + 1;
        } else {
            depth[cur_node] = max_depth;
        }

        // add successors 
        for(auto successor: nodeSuccessors[cur_node]){
            if (!bfs_entries.contains(successor)){
                bfs_queue.push(successor);
                bfs_entries.insert(successor);
            }
        }
    }

    return depth;
}

}  // namespace fuse::passes
