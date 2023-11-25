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
#ifndef FUSE_PLAINTEXTINTERPRETER_H
#define FUSE_PLAINTEXTINTERPRETER_H

#include <functional>
#include <numeric>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

#include "BaseVisitor.h"
#include "ModuleWrapper.h"

namespace fuse::backend {

struct unsupported_operation_error : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct missing_value_error : std::runtime_error {
    using std::runtime_error::runtime_error;
};

using Identifier = uint64_t;

template <typename value_type>
class PlaintextInterpreter {
   public:
    void evaluate(const core::ModuleReadOnly& module, std::unordered_map<Identifier, value_type>& inputMappings);
    void evaluate(const core::CircuitReadOnly& circuit, std::unordered_map<Identifier, value_type>& inputMappings);

   private:
    void evaluate(const core::CircuitReadOnly& circuit, std::unordered_map<Identifier, value_type>& environment, const core::ModuleReadOnly& parentModule);
    void evaluate(const core::NodeReadOnly& node, std::unordered_map<Identifier, value_type>& environment);
    void evaluate(const core::NodeReadOnly& node, std::unordered_map<Identifier, value_type>& environment, const core::ModuleReadOnly& parentModule);
};

/*
 *
 * Function Definitions: Stand-alone Circuit Evaluation
 *
 */

template <typename value_type>
void PlaintextInterpreter<value_type>::evaluate(const core::CircuitReadOnly& circuit, std::unordered_map<Identifier, value_type>& inputMappings) {
    circuit.topologicalTraversal([=, this, &inputMappings](core::NodeReadOnly& node) { this->evaluate(node, inputMappings); });
}

template <typename value_type>
void PlaintextInterpreter<value_type>::evaluate(const core::NodeReadOnly& node, std::unordered_map<Identifier, value_type>& environment) {
    using op = core::ir::PrimitiveOperation;

    // if node has already been computed, return
    if (environment.contains(node.getNodeID())) {
        return;
    }

    // get input values for this node
    std::vector<value_type> inputArgs;
    for (auto id : node.getInputNodeIDs()) {
        if (environment.contains(id)) {
            inputArgs.push_back(environment[id]);
        } else {
            throw missing_value_error("missing input value for Node: " + std::to_string(id) + "\n");
        }
    }

    // compute node output -> start with first input value, accumulate if needed
    assert(!inputArgs.empty());
    value_type eval = inputArgs.at(0);
    auto it = inputArgs.begin() + 1;

    // perform computation depending on operation
    switch (node.getOperation()) {
        case op::Input:
        case op::Output:
            break;

        case op::Constant: {
            eval = node.getConstantFlexbuffer().As<value_type>();
            break;
        }

        case op::Not: {
            assert(inputArgs.size() == 1);
            eval = !eval;
            break;
        }

        case op::And: {
            eval = std::accumulate(it, inputArgs.end(), eval, std::logical_and<value_type>());
            break;
        }

        case op::Xor: {
            eval = std::accumulate(it, inputArgs.end(), eval, std::bit_xor<value_type>());
            break;
        }
        case op::Or: {
            eval = std::accumulate(it, inputArgs.end(), eval, std::logical_or<value_type>());
            break;
        }
        case op::Nand: {
            eval = std::accumulate(it, inputArgs.end(), eval, std::logical_and<value_type>());
            eval = !eval;
            break;
        }

        case op::Nor: {
            eval = std::accumulate(it, inputArgs.end(), eval, std::logical_or<value_type>());
            eval = !eval;
            break;
        }
        case op::Xnor: {
            eval = std::accumulate(it, inputArgs.end(), eval, std::bit_xor<value_type>());
            eval = !eval;
            break;
        }

        case op::Neg: {
            assert(inputArgs.size() == 1);
            eval = -eval;
            break;
        }
        case op::Add: {
            eval = std::accumulate(it, inputArgs.end(), eval, std::plus<value_type>());
            break;
        }
        case op::Mul: {
            eval = std::accumulate(it, inputArgs.end(), eval, std::multiplies<value_type>());
            break;
        }
        case op::Div: {
            eval = std::accumulate(it, inputArgs.end(), eval, std::divides<value_type>());
            break;
        }
        case op::Sub: {
            eval = std::accumulate(it, inputArgs.end(), eval, std::minus<value_type>());
            break;
        }

        case op::Gt: {
            assert(inputArgs.size() == 2);
            eval = *it > *(it + 1);
            break;
        }
        case op::Ge: {
            assert(inputArgs.size() == 2);
            eval = *it >= *(it + 1);
            break;
        }
        case op::Lt: {
            assert(inputArgs.size() == 2);
            eval = *it < *(it + 1);
            break;
        }
        case op::Le: {
            assert(inputArgs.size() == 2);
            eval = *it <= *(it + 1);
            break;
        }
        case op::Eq: {
            assert(inputArgs.size() == 2);
            eval = *it == *(it + 1);
            break;
        }

        case op::CallSubcircuit:
        case op::Loop:
        case op::Split:
        case op::Merge:
        case op::Custom:
        default:
            throw unsupported_operation_error("Unsupported Operation when interpreting Stand-Alone Circuit: " +
                                              std::string(core::ir::EnumNamePrimitiveOperation(node.getOperation())));
    }

    // output is then saved in the mapping for other nodes to use this as their input
    environment[node.getNodeID()] = eval;
}

/*
 *
 * Function Definitions: Complete Module Evaluation
 *
 */
template <typename value_type>
void PlaintextInterpreter<value_type>::evaluate(const core::ModuleReadOnly& module, std::unordered_map<Identifier, value_type>& inputMappings) {
    // visit entry circuit with reference to this module
    // so that calls to subcircuits can be resolved
    auto entryCircuit = module.getEntryCircuit();
    evaluate(*entryCircuit, inputMappings, module);
}

template <typename value_type>
void PlaintextInterpreter<value_type>::evaluate(const core::CircuitReadOnly& circuit, std::unordered_map<Identifier, value_type>& environment, const core::ModuleReadOnly& parentModule) {
    circuit.topologicalTraversal([=, this, &environment, &parentModule](core::NodeReadOnly& node) { this->evaluate(node, environment, parentModule); });
}

template <typename value_type>
void PlaintextInterpreter<value_type>::evaluate(const core::NodeReadOnly& node, std::unordered_map<Identifier, value_type>& environment, const core::ModuleReadOnly& parentModule) {
    using op = core::ir::PrimitiveOperation;

    // if node has already been computed, return
    if (environment.contains(node.getNodeID())) {
        return;
    }

    // get input values for this node
    std::vector<value_type> inputArgs;
    value_type eval = inputArgs.at(0);
    auto it = inputArgs.begin() + 1;
    for (auto id : node.getInputNodeIDs()) {
        if (environment.contains(id)) {
            inputArgs.push_back(environment[id]);
        } else {
            throw missing_value_error("missing input value for Node: " + std::to_string(id) + "\n");
        }
    }

    // perform computation depending on operation
    switch (node.getOperation()) {
        case op::Input:
        case op::Output:
            break;

        case op::Constant: {
            eval = node.getConstantFlexbuffer().As<value_type>();
            break;
        }

        case op::Not: {
            assert(inputArgs.size() == 1);
            eval = !eval;
            break;
        }

        case op::And: {
            eval = std::accumulate(it, inputArgs.end(), eval, std::logical_and<value_type>());
            break;
        }

        case op::Xor: {
            eval = std::accumulate(it, inputArgs.end(), eval, std::bit_xor<value_type>());
            break;
        }
        case op::Or: {
            eval = std::accumulate(it, inputArgs.end(), eval, std::logical_or<value_type>());
            break;
        }
        case op::Nand: {
            eval = std::accumulate(it, inputArgs.end(), eval, std::logical_and<value_type>());
            eval = !eval;
            break;
        }

        case op::Nor: {
            eval = std::accumulate(it, inputArgs.end(), eval, std::logical_or<value_type>());
            eval = !eval;
            break;
        }
        case op::Xnor: {
            eval = std::accumulate(it, inputArgs.end(), eval, std::bit_xor<value_type>());
            eval = !eval;
            break;
        }

        case op::Neg: {
            assert(inputArgs.size() == 1);
            eval = -eval;
            break;
        }
        case op::Add: {
            eval = std::accumulate(it, inputArgs.end(), eval, std::plus<value_type>());
            break;
        }
        case op::Mul: {
            eval = std::accumulate(it, inputArgs.end(), eval, std::multiplies<value_type>());
            break;
        }
        case op::Div: {
            eval = std::accumulate(it, inputArgs.end(), eval, std::divides<value_type>());
            break;
        }
        case op::Sub: {
            eval = std::accumulate(it, inputArgs.end(), eval, std::minus<value_type>());
            break;
        }

        case op::Gt: {
            assert(inputArgs.size() == 2);
            eval = *it > *(it + 1);
            break;
        }
        case op::Ge: {
            assert(inputArgs.size() == 2);
            eval = *it >= *(it + 1);
            break;
        }
        case op::Lt: {
            assert(inputArgs.size() == 2);
            eval = *it < *(it + 1);
            break;
        }
        case op::Le: {
            assert(inputArgs.size() == 2);
            eval = *it <= *(it + 1);
            break;
        }
        case op::Eq: {
            assert(inputArgs.size() == 2);
            eval = *it == *(it + 1);
            break;
        }

        case op::CallSubcircuit: {
            // prepare environment for call to subcircuit
            std::unordered_map<Identifier, value_type> subcircuitInputs;
            auto subcircuit = parentModule.getCircuitWithName(node.getSubCircuitName());

            // we need exact amount of input values for the subcircuit
            assert(inputArgs.size() == subcircuit->getNumberOfInputs());
            size_t in;
            for (auto inputNode : subcircuit->getInputNodeIDs()) {
                subcircuitInputs[inputNode] = inputArgs.at(in++);
            }

            // evaluate subcircuit with the same inputs that the node itself received
            evaluate(*subcircuit, subcircuitInputs, parentModule);

            // after evaluation, get circuit output and save them in this environment as well
            // to do this: translate subcircuit output ID to node output ID
            assert(subcircuit->getNumberOfOutputs() == 1);
            eval = subcircuitInputs.at(subcircuit->getOutputNodeIDs()[0]);
            break;
        }
        case op::Loop:
        case op::Split:
        case op::Merge:
        default:
            throw unsupported_operation_error("Node contains unsupported operation: " +
                                              std::string(core::ir::EnumNamePrimitiveOperation(node.getOperation())));
    }
}

}  // namespace fuse::backend
#endif /* FUSE_PLAINTEXTINTERPRETER_H */
