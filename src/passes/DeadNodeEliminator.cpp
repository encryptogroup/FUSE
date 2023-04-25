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

#include "DeadNodeEliminator.h"

#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace fuse::passes {
class DeadNodeEliminator {
    using Identifier = uint64_t;

   public:
    void visit(core::ModuleObjectWrapper& module, bool removeUnusedCircuits = false);
    void visit(core::CircuitObjectWrapper& circuit);

   private:
    void visit(core::NodeObjectWrapper& node, std::unordered_set<Identifier>& liveNodes, core::CircuitObjectWrapper& parentCircuit);
    std::queue<std::string> workingSet;
    std::unordered_set<std::string> liveCircuits;
};

/*
DeadNodeEliminator Member Functions
 */
void DeadNodeEliminator::visit(core::ModuleObjectWrapper& module, bool removeUnusedCircuits) {
    auto entryCircuit = module.getEntryCircuit();
    workingSet.push(entryCircuit.getName());
    while (!workingSet.empty()) {
        auto currentCircuit = module.getCircuitWithName(workingSet.front());
        workingSet.pop();
        visit(currentCircuit);
    }
    auto circuitNames = module.getAllCircuitNames();
    if (removeUnusedCircuits) {
        for (auto circuit : circuitNames) {
            // if this circuit has been used anywhere from the entry point -> remove it from the module
            if (!liveCircuits.contains(circuit)) {
                module.removeCircuit(circuit);
            }
        }
    }
}

void DeadNodeEliminator::visit(core::CircuitObjectWrapper& circuit) {
    // this circuit has already been visited
    if (liveCircuits.contains(circuit.getName())) {
        return;
    } else {
        liveCircuits.insert(circuit.getName());
    }
    // mark live nodes
    std::unordered_set<Identifier> liveNodes;
    for (auto outputNodeID : circuit.getOutputNodeIDs()) {
        liveNodes.insert(outputNodeID);
        auto outputNode = circuit.getNodeWithID(outputNodeID);
        visit(outputNode, liveNodes, circuit);
    }
    // delete nodes that have not been marked as live -> considered dead nodes
    circuit.removeNodesNotContainedIn(liveNodes);
}

void DeadNodeEliminator::visit(core::NodeObjectWrapper& node, std::unordered_set<Identifier>& liveNodes, core::CircuitObjectWrapper& parentCircuit) {
    if (node.getOperation() == core::ir::PrimitiveOperation::CallSubcircuit) {
        workingSet.push(node.getSubCircuitName());
    }
    for (auto inputNodeID : node.getInputNodeIDs()) {
        liveNodes.insert(inputNodeID);
        auto inputNode = parentCircuit.getNodeWithID(inputNodeID);
        visit(inputNode, liveNodes, parentCircuit);
    }
}

/*
Function Definitions from Header File: Setup DeadNodeEliminator and call visit
 */

void eliminateDeadNodes(core::ModuleObjectWrapper& module, bool removeUnusedCircuits) {
    DeadNodeEliminator eliminator;
    eliminator.visit(module, removeUnusedCircuits);
}

void eliminateDeadNodes(core::CircuitObjectWrapper& circuit) {
    DeadNodeEliminator eliminator;
    eliminator.visit(circuit);
}

}  // namespace fuse::passes
