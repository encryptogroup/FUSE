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

#include "DOTBackend.h"

#include "BaseVisitor.h"

namespace fuse::backend {

class DOTCodeGenerator {
    using Identifier = uint64_t;
    using Offset = uint32_t;

   private:
    // resulting dot code is put together in this stringstream
    // alternatively, one could provide a file stream or something similar
    std::stringstream dot_;

    struct Environment {
        // map node IDs to the strings used in the dot code to generate correct edges
        std::unordered_map<Identifier, std::string> nodeNames;
        std::map<std::pair<Identifier, Offset>, std::string> nodesWithOffset{};
        std::unordered_map<std::string, std::optional<std::string>> lineEnd{};
        std::stringstream dot{};
        std::string circuitName;
    };

    // save call edges here to print them at the end
    std::vector<std::string> callSites;

    // map circuits to their corresponding environment
    std::unordered_map<std::string, Environment> circuitEnv;

    // set color once here to be used in the whole code afterwards
    // colors relative to pastel28 brewer color scheme
    const std::string inputColorWithLineEnd = " [color=1];\n";
    const std::string outputColorWithLineEnd = " [color=2];\n";
    const std::string nodeColorWithLineEnd = " [color=3];\n";
    const std::string callColorWithLineEnd = " [color=4];\n";
    const std::string edgeColorWithLineEnd = " [color=7];\n";

   public:
    std::string
    getDot() const;

    std::string visitType(const core::DataTypeReadOnly& datatype);

    std::string visitInput(const core::NodeReadOnly& node, Environment& env);

    std::string visitOutput(const core::NodeReadOnly& node, Environment& env);

    void visit(const core::NodeReadOnly& node, Environment& env);

    void visit(const core::CircuitReadOnly& circuit, bool omitUnusedIONodes = false);

    /* Dot generation for a whole module with support for calls and subgraphs */

    void visit(const core::NodeReadOnly& node, Environment& env, const core::ModuleReadOnly& parentModule);

    void visit(const core::CircuitReadOnly& circuit, const core::ModuleReadOnly& parentModule, bool omitUnusedIONodes = false);

    void visit(const core::ModuleReadOnly& module, bool omitUnusedIONodes = false);
};

void DOTCodeGenerator::visit(const core::CircuitReadOnly& circuit, const core::ModuleReadOnly& parentModule, bool omitUnusedIONodes) {
    auto& env = circuitEnv[circuit.getName()];
    env.circuitName = circuit.getName();

    // start new subgraph for circuit inside module
    env.dot << "\tsubgraph cluster_" + circuit.getName() +
                   " {\n"
                   "\tnode [style=filled];\n"
                   "\tnode [colorscheme=pastel28];\n"
                   "\tlabel = \"" +
                   circuit.getName() +
                   "\";\n\tcolor=purple;\n";

    // add input nodes
    for (auto input : circuit.getInputNodeIDs()) {
        auto name = visitInput(*circuit.getNodeWithID(input), env);
        env.nodeNames[input] = name;
    }

    // add output nodes
    for (auto output : circuit.getOutputNodeIDs()) {
        auto name = visitOutput(*circuit.getNodeWithID(output), env);
        env.nodeNames[output] = name;
    }

    // generate dot code for the rest of the nodes
    circuit.topologicalTraversal([&, this](core::NodeReadOnly& node) { this->visit(node, env, parentModule); });

    // end subgraph
    env.dot << "}\n\n";
}

void DOTCodeGenerator::visit(const core::ModuleReadOnly& module, bool omitUnusedIONodes) {
    // start new graph for module with subgraphs
    dot_ << "digraph Module"
            " {\n"
            "\tcompound=true;"
            "\tratio = fill;\n"
            "\tnode [style=filled];\n"
            "\tnode [colorscheme=pastel28];\n";

    // generate DOT code for circuits by topological traversal
    auto entry = module.getEntryCircuit();
    visit(*entry, module, omitUnusedIONodes);

    // print all subgraphs
    for (auto& it : circuitEnv) {
        dot_ << it.second.dot.str();
    }
    // then, print all call sites
    // TODO Not working
    for (auto& call : callSites) {
        dot_ << call;
    }

    // end digraph
    dot_ << "}\n";
}

void DOTCodeGenerator::visit(const core::CircuitReadOnly& circuit, bool omitUnusedIONodes) {
    auto& env = circuitEnv[circuit.getName()];
    env.circuitName = circuit.getName();

    // start new graph for circuit
    env.dot << "digraph " + circuit.getName() +
                   " {\n"
                   "\tratio = fill;\n"
                   "\tnode [colorscheme=pastel28];\n"
                   "\tnode [style=filled];\n";

    // add input nodes
    for (auto input : circuit.getInputNodeIDs()) {
        auto name = visitInput(*circuit.getNodeWithID(input), env);
        env.nodeNames[input] = name;
    }

    // add output nodes
    for (auto output : circuit.getOutputNodeIDs()) {
        auto name = visitOutput(*circuit.getNodeWithID(output), env);
        env.nodeNames[output] = name;
    }

    // generate dot code for the rest of the nodes
    circuit.topologicalTraversal([&, this](core::NodeReadOnly& node) { this->visit(node, env); });

    // end digraph
    env.dot << "}\n\n";

    // print dot code from environment into global dot output
    dot_ << env.dot.str();
}

std::string DOTCodeGenerator::visitInput(const core::NodeReadOnly& node, Environment& env) {
    // std::string res = "\"Input " + std::to_string(node.getNodeID()) + " : ";
    std::string res = "\"" + env.circuitName + "_" + std::to_string(node.getNodeID()) + ": ";
    for (auto& type : node.getInputDataTypes()) {
        res += visitType(*type) + ", ";
    }
    res += "\"";
    return res;
}

std::string DOTCodeGenerator::visitOutput(const core::NodeReadOnly& node, Environment& env) {
    // std::string res = "\"Output " + std::to_string(node.getNodeID()) + " : ";
    std::string res = "\"" + env.circuitName + "_" + std::to_string(node.getNodeID()) + ": ";
    for (auto& type : node.getOutputDataTypes()) {
        res += visitType(*type) + ", ";
    }
    res += "\"";
    return res;
}

void DOTCodeGenerator::visit(const core::NodeReadOnly& node, Environment& env) {
    std::string name;
    // input node: print node with color
    if (node.isInputNode()) {
        name = env.nodeNames[node.getNodeID()];
        env.lineEnd[name] = inputColorWithLineEnd;
    }
    // output node: print node with color
    else if (node.isOutputNode()) {
        name = env.nodeNames[node.getNodeID()];
        env.lineEnd[name] = outputColorWithLineEnd;
    }
    // constant node: print node with constant value
    else if (node.isConstantNode()) {
        name = std::to_string(node.getNodeID()) + ": ";
        std::string constantValue = node.getConstantFlexbuffer().ToString();
        name = "\"" + env.circuitName + "_" + name + constantValue + "\"";
        env.nodeNames[node.getNodeID()] = name;
        env.lineEnd[name] = nodeColorWithLineEnd;
    }
    // other node: print node with operations
    else {
        name = std::to_string(node.getNodeID()) + ": ";
        std::string opName = core::ir::EnumNamePrimitiveOperation(node.getOperation());
        std::string nameWoQuotes = env.circuitName + "_" + name + opName;
        name = "\"" + nameWoQuotes + "\"";
        env.nodeNames[node.getNodeID()] = name;
        env.lineEnd[name] = nodeColorWithLineEnd;

        // nodes with more than 1 output: create node for every output
        // connect operation with output and use the offsetted outputs for the inputs of nodes that use the offsets
        if (node.getNumberOfOutputs() > 1) {
            Identifier id = node.getNodeID();
            for (int i = 0; i < node.getNumberOfOutputs(); ++i) {
                std::string offsetName = "\"" + nameWoQuotes + "[" + std::to_string(i) + "]\"";
                env.nodesWithOffset[std::make_pair(id, i)] = offsetName;
                env.lineEnd[offsetName] = nodeColorWithLineEnd + name + nodeColorWithLineEnd +  // offset node itself and parent node
                                                                                                // arrow from node to offset
                                          env.nodeNames[id] + " -> " + env.nodesWithOffset[std::make_pair(id, i)] + edgeColorWithLineEnd;
            }
        }
    }

    // add edges for inputs to this node
    for (size_t input = 0; input < node.getNumberOfInputs(); ++input) {
        auto inputID = node.getInputNodeIDs()[input];
        std::string inputName;
        if (node.usesInputOffsets()) {
            auto offset = node.getInputOffsets()[input];
            if (env.nodesWithOffset.contains(std::make_pair(inputID, offset))) {
                inputName = env.nodesWithOffset[std::make_pair(inputID, offset)];
            } else {
                // fallback if input node does not have multiple outputs
                inputName = env.nodeNames[inputID];
            }
        } else {
            // if input has only one output: print input node name directly
            inputName = env.nodeNames[inputID];
        }

        auto& inOptional = env.lineEnd[inputName];
        if (inOptional.has_value()) {
            env.dot << inputName << inOptional.value();
            inOptional.reset();
        }

        env.dot << inputName << " -> " << name << edgeColorWithLineEnd;
    }
}

void DOTCodeGenerator::visit(const core::NodeReadOnly& node, Environment& env, const core::ModuleReadOnly& parentModule) {
    if (node.isSubcircuitNode()) {
        auto callee = node.getSubCircuitName();
        // check if subcircuit DOT code has already been generated
        if (!circuitEnv.contains(callee)) {
            // generate dot code for callee first
            auto subcirc = parentModule.getCircuitWithName(callee);
            visit(*subcirc, parentModule);
        }
        // add call node to DOT
        std::string name = std::to_string(node.getNodeID()) + ": Call " + callee;
        std::string nameWoQuotes = env.circuitName + "_" + name;
        name = "\"" + nameWoQuotes + "\"";
        env.nodeNames[node.getNodeID()] = name;
        env.lineEnd[name] = callColorWithLineEnd;

        // create node for every output of the call and connect with this node
        Identifier id = node.getNodeID();
        for (int i = 0; i < node.getNumberOfOutputs(); ++i) {
            std::string offsetName = "\"" + nameWoQuotes + "[" + std::to_string(i) + "]\"";
            env.nodesWithOffset[std::make_pair(id, i)] = offsetName;
            env.lineEnd[offsetName] = callColorWithLineEnd + name + callColorWithLineEnd +  // call offset node itself
                                                                                            // arrow from call node to offset node
                                      env.nodeNames[id] + " -> " + env.nodesWithOffset[std::make_pair(id, i)] + edgeColorWithLineEnd;
        }

        // add edges for inputs to call site
        for (size_t input = 0; input < node.getNumberOfInputs(); ++input) {
            auto inputID = node.getInputNodeIDs()[input];
            std::string inputName;
            if (node.usesInputOffsets()) {
                auto offset = node.getInputOffsets()[input];
                if (env.nodesWithOffset.contains(std::make_pair(inputID, offset))) {
                    inputName = env.nodesWithOffset[std::make_pair(inputID, offset)];
                } else {
                    // fallback if input node does not have multiple outputs
                    inputName = env.nodeNames[inputID];
                }
            } else {
                // if input has only one output: print input node name directly
                inputName = env.nodeNames[inputID];
            }

            auto& inOpt = env.lineEnd[inputName];
            if (inOpt.has_value()) {
                env.dot << inputName << inOpt.value();
                inOpt.reset();
            }

            env.dot << inputName << " -> " << name << edgeColorWithLineEnd;
        }
        // add special edge from caller to callee
        auto calleeCirc = parentModule.getCircuitWithName(callee);
        uint64_t calleeNode = calleeCirc->getInputNodeIDs()[0];
        const std::string& calleeName = circuitEnv[callee].nodeNames[calleeNode];
        std::string callEdge = name + " -> " + calleeName + "[lhead=cluster_" + callee + ",color=7];\n";
        callSites.push_back(callEdge);
    } else {
        visit(node, env);
    }
}

std::string DOTCodeGenerator::visitType(const core::DataTypeReadOnly& datatype) {
    std::string sl = datatype.getSecurityLevelName();
    std::string pt = datatype.getPrimitiveTypeName();
    return "(" + sl + " : " + pt + ")";
}

std::string DOTCodeGenerator::getDot() const {
    return dot_.str();
}

std::string generateDotCodeFrom(const core::CircuitReadOnly& circuit, bool omitUnusedIONodes) {
    DOTCodeGenerator dotGenerator;
    dotGenerator.visit(circuit, omitUnusedIONodes);
    return dotGenerator.getDot();
}

std::string generateDotCodeFrom(const core::ModuleReadOnly& module, bool omitUnusedIONode) {
    DOTCodeGenerator dotGenerator;
    dotGenerator.visit(module, omitUnusedIONode);
    return dotGenerator.getDot();
}
}  // namespace fuse::backend
