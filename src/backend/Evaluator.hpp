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
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED
 * "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <any>
#include <functional>
#include <numeric>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "BaseVisitor.h"
#include "ModuleWrapper.h"

#ifndef FUSE_EVALUATOR_H
#define FUSE_EVALUATOR_H

namespace fuse::backend::experimental {
    using Identifier = uint64_t;
    using Offset = uint32_t;

    /*
    Function Declarations
    */

    std::any evaluateConstantNode(const core::NodeReadOnly &node) {
        using type = core::ir::PrimitiveType;
        auto datatype = node.getInputDataTypes().at(0).get();
        switch (datatype->getPrimitiveType()) {
        case type::Bool:
            if (datatype->isPrimitiveType()) {
                return node.getConstantBool();
            } else {
                return node.getConstantBoolVector();
            }
        case type::Int8:
            if (datatype->isPrimitiveType()) {
                return node.getConstantInt8();
            } else {
                return node.getConstantInt8Vector();
            }
        case type::Int16:
            if (datatype->isPrimitiveType()) {
                return node.getConstantInt16();
            } else {
                return node.getConstantInt16Vector();
            }
        case type::Int32:
            if (datatype->isPrimitiveType()) {
                return node.getConstantInt32();
            } else {
                return node.getConstantInt32Vector();
            }
        case type::Int64:
            if (datatype->isPrimitiveType()) {
                return node.getConstantInt64();
            } else {
                return node.getConstantInt64Vector();
            }
        case type::UInt8:
            if (datatype->isPrimitiveType()) {
                return node.getConstantUInt8();
            } else {
                return node.getConstantUInt8Vector();
            }
        case type::UInt16:
            if (datatype->isPrimitiveType()) {
                return node.getConstantUInt16();
            } else {
                return node.getConstantUInt16Vector();
            }
        case type::UInt32:
            if (datatype->isPrimitiveType()) {
                return node.getConstantUInt32();
            } else {
                return node.getConstantUInt32Vector();
            }
        case type::UInt64:
            if (datatype->isPrimitiveType()) {
                return node.getConstantUInt64();
            } else {
                return node.getConstantUInt64Vector();
            }
        case type::Float:
            if (datatype->isPrimitiveType()) {
                return node.getConstantFloat();
            } else {
                return node.getConstantFloatVector();
            }
        case type::Double:
            if (datatype->isPrimitiveType()) {
                return node.getConstantDouble();
            } else {
                return node.getConstantDoubleVector();
            }
        default:
            // this line should be unreachable
            throw std::logic_error("invalid type for constant: " +
                                   datatype->getPrimitiveTypeName());
        }
    }

    void evaluate(
        const core::NodeReadOnly &node,
        std::unordered_map<Identifier, std::vector<std::any>> &environment) {
        using op = core::ir::PrimitiveOperation;

        // if node has already been computed, return
        if (environment.contains(node.getNodeID())) {
            return;
        }
        std::vector<std::any> inputValues;
        int offset = 0;
        for (auto id : node.getInputNodeIDs()) {
            if (environment.contains(id)) {
                if (node.usesInputOffsets()) {
                    inputValues.push_back(environment.at(id).at(node.getInputOffsets()[offset]));
                    ++offset;
                } else {
                    inputValues.push_back(environment.at(id).at(0));
                }
            } else {
                throw std::logic_error(
                    "missing input value for Node: " + std::to_string(id) + "\n");
            }
        }

        std::vector<std::any> outputValues;
        switch (node.getOperation()) {
        case op::Input:
            assert(inputValues.size() == 1);
            break;
        case op::Output:
            assert(inputValues.size() == 1);
            outputValues.push_back(inputValues.at(0));
        case op::Constant: {
            outputValues.push_back(evaluateConstantNode(node));
            break;
        }
        case op::Not: {
            assert(inputValues.size() == 1);
            outputValues.push_back(!std::any_cast<bool>(inputValues.at(0)));
        }
        case op::And: {
            assert(inputValues.size() == 2);
            outputValues.push_back(std::any_cast<bool>(inputValues.at(0)) &&
                                   std::any_cast<bool>(inputValues.at(1)));
            break;
        }
        case op::Xor: {
            assert(inputValues.size() == 2);
            outputValues.push_back(std::any_cast<bool>(inputValues.at(0)) ^
                                   std::any_cast<bool>(inputValues.at(1)));
            break;
        }
        case op::Or: {
            assert(inputValues.size() == 2);
            outputValues.push_back(std::any_cast<bool>(inputValues.at(0)) ||
                                   std::any_cast<bool>(inputValues.at(1)));
            break;
        }
        case op::Nand: {
            assert(inputValues.size() == 2);
            outputValues.push_back(!(std::any_cast<bool>(inputValues.at(0)) &&
                                     std::any_cast<bool>(inputValues.at(1))));
            break;
        }
        case op::Nor: {
            assert(inputValues.size() == 2);
            outputValues.push_back(!(std::any_cast<bool>(inputValues.at(0)) ||
                                     std::any_cast<bool>(inputValues.at(1))));
            break;
        }
        case op::Xnor: {
            assert(inputValues.size() == 2);
            outputValues.push_back(!(std::any_cast<bool>(inputValues.at(0)) ^
                                     std::any_cast<bool>(inputValues.at(1))));
            break;
        }

        // Arithmetic Operations
        case op::Add: {
            assert(inputValues.size() == 2);
            outputValues.push_back((std::any_cast<int32_t>(inputValues.at(0)) +
                                    std::any_cast<int32_t>(inputValues.at(1))));
            break;
        }
        case op::Mul: {
            assert(inputValues.size() == 2);
            outputValues.push_back((std::any_cast<int32_t>(inputValues.at(0)) *
                                    std::any_cast<int32_t>(inputValues.at(1))));
            break;
        }
        case op::Sub: {
            assert(inputValues.size() == 2);
            outputValues.push_back((std::any_cast<int32_t>(inputValues.at(0)) -
                                    std::any_cast<int32_t>(inputValues.at(1))));
            break;
        }
        case op::Gt: {
            assert(inputValues.size() == 2);
            outputValues.push_back((std::any_cast<int32_t>(inputValues.at(0)) >
                                    std::any_cast<int32_t>(inputValues.at(1))));
            break;
        }
        case op::Split: {
            assert(inputValues.size() == 1);
            std::bitset<32> bits = std::any_cast<int32_t>(inputValues.at(0));
            // vector: (msb [31] | ... | lsb [0])
            for (int i = bits.size() - 1; i >= 0; --i) {
                outputValues.push_back(bits[i]);
            }
            break;
        }
        case op::Merge: {
            assert(inputValues.size() == 32);
            std::bitset<32> bits = 0;
            for (int i = 0; i < bits.size(); ++i) {
                bits[i] = std::any_cast<bool>(inputValues.at(i));
            }
            outputValues.push_back(std::stoi(bits.to_string()));
            break;
        }
        default: {
            throw std::runtime_error("Unsupported Operation: " +
                                     node.getOperationName());
        }
        }

        // here we have the outputs defined --> update map
        environment.at(node.getNodeID()) = outputValues;
    }

    void evaluate(
        const core::CircuitReadOnly &circuit,
        std::unordered_map<Identifier, std::vector<std::any>> &environment) {
        circuit.topologicalTraversal(
            [=, &environment](const core::NodeReadOnly &node) {
                evaluate(node, environment);
            });
    }

    void evaluate(
        const core::CircuitReadOnly &circuit,
        const core::ModuleReadOnly &parentModule,
        std::unordered_map<Identifier, std::vector<std::any>> &environment) {
        circuit.topologicalTraversal(
            [=, &parentModule, &environment](const core::NodeReadOnly &node) {
                if (node.isSubcircuitNode()) {
                    // evaluateCall:
                    auto subcircName = node.getSubCircuitName();
                    auto calleeCirc = parentModule.getCircuitWithName(subcircName);
                    auto inputVals = node.getInputNodeIDs();
                    std::vector<std::any> inputs;
                    for (auto in : inputVals) {
                        inputs.push_back(environment.at(in));
                    }
                    std::unordered_map<Identifier, std::vector<std::any>> calleeEnv{};
                    auto calleeInputIDs = calleeCirc->getInputNodeIDs();
                    assert(inputs.size() == calleeInputIDs.size());
                    for (int i = 0; i < inputs.size(); ++i) {
                        // don't ask me but this should work (hopefully)
                        calleeEnv[calleeInputIDs[i]].push_back(inputs.at(i));
                    }
                    // evaluate subcircuit
                    evaluate(*calleeCirc.get(), parentModule, calleeEnv);
                    // read out output values from call
                    std::vector<std::any> outputs;
                    for (auto calleeOutputID : calleeCirc->getOutputNodeIDs()) {
                        auto outputVec = calleeEnv[calleeOutputID];
                        assert(outputVec.size() == 1);
                        outputs.push_back(outputVec.at(0));
                    }
                    // update map
                    environment[node.getNodeID()] = outputs;
                } else {
                    evaluate(node, environment);
                }
            });
    }

    void evaluate(
        const core::ModuleReadOnly &parentModule,
        std::unordered_map<Identifier, std::vector<std::any>> &environment) {
        auto circuit = parentModule.getEntryCircuit();
        evaluate(*circuit.get(), parentModule, environment);
    }

} // namespace fuse::backend::experimental
#endif /* FUSE_EVALUATOR_H */
