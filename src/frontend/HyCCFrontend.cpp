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

#include "HyCCFrontend.h"

#include <ModuleBuilder.h>
#include <libcircuit/simple_circuit.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace fuse::frontend {

/*
****************************************************
****************************************************
* HyCC Adapter for translating with function calls *
****************************************************
****************************************************
*/

struct HyCCcircuitContext {
    // Map from HyCC wire -> FUSE Node ID
    std::unordered_map<simple_circuitt::gatet::wire_endpointt,
                       fuse::frontend::Identifier, wire_endpoint_hasht>
        wireEndpointToID_;

    // Map from HyCC Wire -> FUSE Node Output Offset used as input for a new FUSE Node
    std::unordered_map<simple_circuitt::gatet::wire_endpointt,
                       uint32_t, wire_endpoint_hasht>
        wireEndpointToOffset_;
};

class HyCCAdapterWithCalls {
    using Offset = uint32_t;
    using Identifier = uint64_t;
    // FUSE Module Builder
    ModuleBuilder moduleBuilder_;
    // Map for each circuit name to its circuit context to be able to add Call Nodes
    std::unordered_map<std::string, HyCCcircuitContext> circuitContexts_;
    // HyCC Logger for constructing HyCC's simple_circuitt objects from the binaries
    loggert hyccLogger_;
    // maps each of the circuit names to their corresponding in-memory representation from HyCC
    std::unordered_map<std::string, simple_circuitt> hyccCircuits_;
    std::ofstream debugOut{"../../tests/outputs/hycc_frontend/debug.txt", std::ofstream::out};

    ir::PrimitiveType
    getPrimitiveType(simple_circuitt::gatet *gate);
    std::optional<simple_circuitt::function_callt> getUnresolvedFunctionCall(const std::string &circuitName, const simple_circuitt::gatet::wire_endpointt &unresolvedFanin);
    void checkGateInputValuesPresent(const std::string &circuitName, simple_circuitt::gatet &gate);

    void translateGate(const std::string &circuitName, simple_circuitt::gatet &gate, simple_circuitt::GATE_OP op);
    void translateCircuitInputs(const std::string &circuitName);
    void translateCircuitOutputs(const std::string &circuitName);
    void translateFunctionCall(const std::string &circuitName, simple_circuitt::function_callt functionCall);
    void visitUnaryGate(const std::string &circuitName, simple_circuitt::gatet &gate, simple_circuitt::GATE_OP op);
    void visitBinaryGate(const std::string &circuitName, simple_circuitt::gatet &gate, simple_circuitt::GATE_OP op);
    void visitCombineGate(const std::string &circuitName, simple_circuitt::gatet &gate);
    void visitSplitGate(const std::string &circuitName, simple_circuitt::gatet &gate);
    void visitConstantGate(const std::string &circuitName, simple_circuitt::gatet &gate);
    Identifier findNodeForWire(const std::string &circuitName, const simple_circuitt::gatet::wire_endpointt &wire);
    Offset findOffsetForWire(const std::string &circuitName, const simple_circuitt::gatet::wire_endpointt &wire);

    // For debugging purposes
    void printGateInputs(const std::string &circuitName, simple_circuitt::gatet &gate);
    void printGateOutputs(const std::string &circuitName, simple_circuitt::gatet &gate);

   public:
    // name of the main circuit in HyCC
    std::string hyccMainName_ = "mpc_main";
    // The directory that contains all .circ files
    std::filesystem::path hyccCircuitDirectory;
    // The name of all circuits inside the circuit_directory_
    std::vector<std::string> hyccCircuitFiles;
    // only load the HyCC circuits without linking. This will add entries to hyccCircuits_
    void loadCircFiles();
    // Process a HyCC Circuit, creating a HyCCcircuitContext and translating it to FUSE IR.
    void processHyCCcircuit(const std::string &circuitName);

    core::ModuleContext getFinishedModuleContext() {
        return core::ModuleContext(moduleBuilder_);
    }
    void writeFUSEToFile(const std::string &pathToSaveBuffer) {
        moduleBuilder_.finishAndWriteToFile(pathToSaveBuffer);
    }
};

Identifier HyCCAdapterWithCalls::findNodeForWire(const std::string &circuitName, const simple_circuitt::gatet::wire_endpointt &wire) {
    auto circIt = circuitContexts_.find(circuitName);
    if (circIt == circuitContexts_.end()) {
        throw std::logic_error("Could not find circuit context for: " + circuitName);
    } else {
        auto idIt = circIt->second.wireEndpointToID_.find(wire);
        if (idIt == circIt->second.wireEndpointToID_.end()) {
            throw std::logic_error("Could not find node ID for wire: " + wire.gate->label + " in circuit: " + circuitName);
        }
        return idIt->second;
    }
}
HyCCAdapterWithCalls::Offset HyCCAdapterWithCalls::findOffsetForWire(const std::string &circuitName, const simple_circuitt::gatet::wire_endpointt &wire) {
    auto circIt = circuitContexts_.find(circuitName);
    if (circIt == circuitContexts_.end()) {
        throw std::logic_error("Could not find circuit context for: " + circuitName);
    } else {
        auto idIt = circIt->second.wireEndpointToOffset_.find(wire);
        if (idIt == circIt->second.wireEndpointToOffset_.end()) {
            return 0;
        }
        return idIt->second;
    }
}

ir::PrimitiveType HyCCAdapterWithCalls::getPrimitiveType(simple_circuitt::gatet *gate) {
    if (gate->get_width() == 1) {
        return ir::PrimitiveType::Bool;
    } else if (gate->get_width() <= 8) {
        return ir::PrimitiveType::UInt8;
    } else if (gate->get_width() <= 16) {
        return ir::PrimitiveType::UInt16;
    } else if (gate->get_width() <= 32) {
        return ir::PrimitiveType::UInt32;
    } else if (gate->get_width() <= 64) {
        return ir::PrimitiveType::UInt64;
    } else {
        throw hycc_error("unexpected bit width: " + gate->get_width());
    }
}

void HyCCAdapterWithCalls::translateCircuitInputs(const std::string &circuitName) {
    CircuitBuilder *circuitBuilder = moduleBuilder_.getCircuitFromName(circuitName);
    simple_circuitt &hyccCirc = hyccCircuits_.at(circuitName);
    HyCCcircuitContext &context = circuitContexts_[circuitName];

    // ordered inputs are of type variablet*
    for (const auto *input_var : hyccCirc.ordered_inputs()) {
        const auto &label = input_var->name;

        // owner is either: { input_alice, input_bob, output }
        const auto input_owner = input_var->owner;

        std::string input_annotation;
        ir::SecurityLevel securityLevel = ir::SecurityLevel::Secure;
        switch (input_owner) {
            case variable_ownert::input_alice: {
                input_annotation = "owner:1";
                break;
            }
            case variable_ownert::input_bob: {
                input_annotation = "owner:2";
                break;
            }
            case variable_ownert::output: {
                input_annotation = label;
                securityLevel = ir::SecurityLevel::Plaintext;
                break;
            }
            default: {
                throw hycc_error("Unknown HyCC Owner Type");
                break;
            }
        }
        // get gates for input variable
        const auto num_gates = input_var->gates.size();
        for (std::size_t i = 0; i < num_gates; ++i) {
            // get current gate that is part of the variable
            auto current_gate = input_var->gates.at(i);
            size_t datatype_offset = circuitBuilder->addDataType(getPrimitiveType(current_gate), securityLevel, {}, input_annotation);
            // get wire_endpoint that defines the output for this variable
            auto gate_output = primary_output(current_gate);
            context.wireEndpointToID_[gate_output] = circuitBuilder->addInputNode({datatype_offset}, input_annotation);
        }
    }
}

void HyCCAdapterWithCalls::translateCircuitOutputs(const std::string &circuitName) {
    CircuitBuilder *circuitBuilder = moduleBuilder_.getCircuitFromName(circuitName);
    simple_circuitt &hyccCirc = hyccCircuits_.at(circuitName);
    HyCCcircuitContext &context = circuitContexts_.at(circuitName);

    for (const auto *output_var : hyccCirc.ordered_outputs()) {
        const auto &label = output_var->name;
        auto &gates = output_var->gates;

        const auto num_gates = output_var->gates.size();

        // output var is split into these gates
        // -> just create gates for each output gate
        for (std::size_t i = 0; i < num_gates; ++i) {
            std::vector<uint64_t> datatype_offsets;
            simple_circuitt::gatet *current_gate = gates.at(i);

            // NOTE: output gates have only ONE fanin
            simple_circuitt::gatet::wire_endpointt single_fanin = current_gate->fanins.at(0);
            // Output depends on wire endpoint inside another function
            if (!context.wireEndpointToID_.contains(single_fanin)) {
                // missing input
                auto func_call_opt = getUnresolvedFunctionCall(circuitName, single_fanin);
                if (func_call_opt.has_value()) {
                    // Generate FUSE circuit for callee first so that its Nodes are available when translating the call
                    processHyCCcircuit(func_call_opt->name);
                    // Now, there is definitively a FUSE circuit -> Now translate inputs and outputs of the function call
                    translateFunctionCall(circuitName, *func_call_opt);
                } else {
                    wire_endpoint_hasht hasher;
                    throw std::logic_error("Missing input value for wire with hash: " + std::to_string(hasher(single_fanin)));
                }
            }
            size_t datatype_offset = circuitBuilder->addDataType(getPrimitiveType(single_fanin.gate), ir::SecurityLevel::Plaintext);
            datatype_offsets.push_back(datatype_offset);

            auto input_node = findNodeForWire(circuitName, single_fanin);
            // context.wireEndpointToID_[input_key] = circuitBuilder->addOutputNode(datatype_offsets, {input_node});
            circuitBuilder->addOutputNode(datatype_offsets, {input_node});
        }
    }
}

void HyCCAdapterWithCalls::visitUnaryGate(const std::string &circuitName, simple_circuitt::gatet &gate, simple_circuitt::GATE_OP op) {
    HyCCcircuitContext &context = circuitContexts_.at(circuitName);
    auto *circuitBuilder = moduleBuilder_.getCircuitFromName(circuitName);
    if (gate.num_fanins() != 1) {
        throw std::logic_error("unary gate received unexpected number of inputs " + gate.num_fanins());
    }

    auto &fanin = gate.fanin_range()[0];
    auto fanout = primary_output(&gate);
    Identifier id_input = findNodeForWire(circuitName, fanin);
    Offset offset = findOffsetForWire(circuitName, fanin);
    Identifier id_output;

    switch (op) {
        case simple_circuitt::NOT: {
            id_output = circuitBuilder->addNode(ir::PrimitiveOperation::Not, {id_input}, {offset});
            break;
        }
        case simple_circuitt::NEG: {
            id_output = circuitBuilder->addNode(ir::PrimitiveOperation::Neg, {id_input}, {offset});
            break;
        }
        default:
            throw std::logic_error("expected unary operation, unexpected operation: " + op);
    }

    context.wireEndpointToID_[fanout] = id_output;
}

void HyCCAdapterWithCalls::visitBinaryGate(const std::string &circuitName, simple_circuitt::gatet &gate, simple_circuitt::GATE_OP op) {
    HyCCcircuitContext &context = circuitContexts_.at(circuitName);
    auto *circuitBuilder = moduleBuilder_.getCircuitFromName(circuitName);

    if (gate.num_fanins() != 2) {
        throw std::logic_error("binary gate received unexpected number of inputs " + gate.num_fanins());
    }

    auto &fanin_a = gate.fanin_range()[0];
    auto &fanin_b = gate.fanin_range()[1];
    auto fanout = primary_output(&gate);

    Identifier id_a = findNodeForWire(circuitName, fanin_a);
    Identifier id_b = findNodeForWire(circuitName, fanin_b);
    Offset offset_a = findOffsetForWire(circuitName, fanin_a);
    Offset offset_b = findOffsetForWire(circuitName, fanin_b);
    Identifier id_output;

    switch (op) {
        case simple_circuitt::AND: {
            id_output = circuitBuilder->addNode(ir::PrimitiveOperation::And, {id_a, id_b}, {offset_a, offset_b});
            break;
        }
        case simple_circuitt::XOR: {
            id_output = circuitBuilder->addNode(ir::PrimitiveOperation::Xor, {id_a, id_b}, {offset_a, offset_b});
            break;
        }
        case simple_circuitt::OR: {
            id_output = circuitBuilder->addNode(ir::PrimitiveOperation::Or, {id_a, id_b}, {offset_a, offset_b});
            break;
        }
        case simple_circuitt::ADD: {
            id_output = circuitBuilder->addNode(ir::PrimitiveOperation::Add, {id_a, id_b}, {offset_a, offset_b});
            break;
        }
        case simple_circuitt::SUB: {
            id_output = circuitBuilder->addNode(ir::PrimitiveOperation::Sub, {id_a, id_b}, {offset_a, offset_b});
            break;
        }
        case simple_circuitt::MUL: {
            id_output = circuitBuilder->addNode(ir::PrimitiveOperation::Mul, {id_a, id_b}, {offset_a, offset_b});
            break;
        }
        default:
            throw std::logic_error("expected unary operation, unexpected operation: " + op);
    }

    context.wireEndpointToID_[fanout] = id_output;
}

void HyCCAdapterWithCalls::visitCombineGate(const std::string &circuitName, simple_circuitt::gatet &gate) {
    HyCCcircuitContext &context = circuitContexts_.at(circuitName);
    auto *circuitBuilder = moduleBuilder_.getCircuitFromName(circuitName);

    // get output key for the wire_endpoint_to_identifier_ map
    auto fanout = primary_output(&gate);
    Identifier id_output;

    // gather all input IDs for combine gate
    std::vector<Identifier> input_nodes;
    std::vector<Offset> input_offsets;
    for (auto &fanin : gate.fanin_range()) {
        input_nodes.push_back(findNodeForWire(circuitName, fanin));
        input_offsets.push_back(findOffsetForWire(circuitName, fanin));
    }

    // generate the FUSE node with a custom operation and correct custom operation name
    id_output = circuitBuilder->addNode(ir::PrimitiveOperation::Merge, input_nodes);

    // store node ID to wire map
    context.wireEndpointToID_[fanout] = id_output;
}
void HyCCAdapterWithCalls::visitSplitGate(const std::string &circuitName, simple_circuitt::gatet &gate) {
    HyCCcircuitContext &context = circuitContexts_.at(circuitName);
    auto *circuitBuilder = moduleBuilder_.getCircuitFromName(circuitName);

    if (gate.num_fanins() != 1) {
        throw std::logic_error("split gate received unexpected numberof inputs: " + gate.num_fanins());
    }

    auto fanin = gate.fanin_range()[0];
    Identifier id_input = findNodeForWire(circuitName, fanin);
    Identifier id_output = circuitBuilder->addNode(ir::PrimitiveOperation::Split, {id_input});

    // split always splits to boolean gates according to HyCC code commentary
    // see https://gitlab.com/securityengineering/HyCC/-/blob/master/src/libcircuit/simple_circuit.cpp#L355
    assert(gate.get_fanouts().size() > 1);
    for (unsigned int wire_i = 0; wire_i < gate.get_fanouts().size(); ++wire_i) {
        simple_circuitt::gatet::wire_endpointt wire_endpoint({&gate, wire_i});
        context.wireEndpointToID_[wire_endpoint] = id_output;
        context.wireEndpointToOffset_[wire_endpoint] = wire_i;
    }
}
void HyCCAdapterWithCalls::visitConstantGate(const std::string &circuitName, simple_circuitt::gatet &gate) {
    HyCCcircuitContext &context = circuitContexts_.at(circuitName);
    auto *circuitBuilder = moduleBuilder_.getCircuitFromName(circuitName);

    // get output key for the wireEndpointToID_ map
    auto fanout = primary_output(&gate);
    Identifier id_output;

    // generate the FUSE node with a constant value
    // ONE is meant to be a boolean gate, which is why the value "true" will be saved
    switch (gate.get_operation()) {
        case simple_circuitt::ONE: {
            id_output = circuitBuilder->addConstantNodeWithPayload(true);
            break;
        }
        case simple_circuitt::CONST: {
            id_output = circuitBuilder->addConstantNodeWithPayload(gate.get_value());
            break;
        }
        default:
            throw std::logic_error("expected combine or split operation, unexpected operation " + gate.get_operation());
    }

    context.wireEndpointToID_[fanout] = id_output;
}

/**
 * @brief If gate depends on function call, return the call on which it depends
 *
 * @param circuitName name of the circuit that is currently translated
 * @param gate gate to check for function call dependencies
 * @return std::optional<std::string>
 */
std::optional<simple_circuitt::function_callt> HyCCAdapterWithCalls::getUnresolvedFunctionCall(const std::string &circuitName, const simple_circuitt::gatet::wire_endpointt &unresolvedFanin) {
    simple_circuitt &callerCirc = hyccCircuits_.at(circuitName);
    HyCCcircuitContext &context = circuitContexts_.at(circuitName);
    if (callerCirc.function_calls().empty()) {
        return std::nullopt;
    }
    // a gate has a dependency on a function call, if the fanins of the gate are provided by the calls outputs
    for (auto func_call : callerCirc.function_calls()) {
        for (auto retVar : func_call.returns) {
            for (auto retGate : retVar.gates) {
                assert(retGate->get_operation() == simple_circuitt::INPUT);
                if (unresolvedFanin == primary_output(retGate)) {
                    return func_call;
                }
            }
        }
    }
    return std::nullopt;
}

void HyCCAdapterWithCalls::translateFunctionCall(const std::string &circuitName, simple_circuitt::function_callt functionCall) {
    // similar to circuit input and output handling for circuits
    // translate not the variables directly, but the gates for each variable
    auto circuitBuilder = moduleBuilder_.getCircuitFromName(circuitName);
    auto &context = circuitContexts_.at(circuitName);
    auto &calleeContext = circuitContexts_.at(circuitName);

    std::vector<size_t> inputDatatypeOffsets;
    std::vector<uint64_t> inputNodeIDs;
    std::vector<uint32_t> inputNodeOffsets;

    wire_endpoint_hasht hasher;

    // std::cout << "[DEBUG] reached function call with callee: " << functionCall.name << std::endl;
    // translate input variables gate by gate
    // -> save input Node IDs and input datatypes
    for (const auto argVar : functionCall.args) {
        const auto num_gates = argVar.gates.size();
        for (const auto argGate : argVar.gates) {
            assert(argGate->get_operation() == simple_circuitt::OUTPUT);
            // get the current gate
            size_t datatype_offset = circuitBuilder->addDataType(getPrimitiveType(argGate));
            // save input type for call node
            inputDatatypeOffsets.push_back(datatype_offset);
            // get wire_endpoint that defines the output for this gate
            auto single_fanin = argGate->fanins.at(0);
            // the function inputs themselves can again be dependent on other function calls
            // --> visit the dependency first!
            checkGateInputValuesPresent(circuitName, *argGate);

            inputNodeIDs.push_back(context.wireEndpointToID_.at(single_fanin));
            // if input is accesses with offset, save the offset, else save 0 as no offset
            if (context.wireEndpointToOffset_.contains(single_fanin)) {
                inputNodeOffsets.push_back(context.wireEndpointToOffset_.at(single_fanin));
            } else {
                inputNodeOffsets.push_back(0);
            }
        }
    }

    std::vector<size_t> outputDatatypeOffsets;
    std::vector<simple_circuitt::gatet::wire_endpointt> callOutputWires;

    // translate output variables gate by gate
    // count the number of output gates -> number of outputs of the call node
    // save output datatypes
    for (const auto retVar : functionCall.returns) {
        for (const auto retGate : retVar.gates) {
            assert(retGate->get_operation() == simple_circuitt::INPUT);
            // translate each output gate separately
            auto type = getPrimitiveType(retGate);
            auto fanout = primary_output(retGate);
            // save this fanin to map it later to the Call Node with the correct offset
            callOutputWires.push_back(fanout);
            size_t typeOffset = circuitBuilder->addDataType(getPrimitiveType(fanout.gate));
            outputDatatypeOffsets.push_back(typeOffset);
        }
    }

    const auto calleeName = functionCall.name;

    // construct call node with inputs, outputs, and callee name
    auto callNodeID = circuitBuilder->addCallToSubcircuitNode(inputDatatypeOffsets,
                                                              inputNodeIDs,
                                                              inputNodeOffsets,
                                                              calleeName,
                                                              outputDatatypeOffsets);

    // then map all HyCC output wires to the call node + their offset
    size_t current_offset = 0;

    for (auto fanout : callOutputWires) {
        context.wireEndpointToID_[fanout] = callNodeID;
        context.wireEndpointToOffset_[fanout] = current_offset;
        ++current_offset;
    }

    // std::cout << "[SUCCESS] translated function call with callee: " << functionCall.name << std::endl;
}

void HyCCAdapterWithCalls::checkGateInputValuesPresent(const std::string &circuitName, simple_circuitt::gatet &gate) {
    HyCCcircuitContext &context = circuitContexts_[circuitName];
    // Check if any input values are missing due to unresolved function calls
    for (auto fanin : gate.fanin_range()) {
        if (!context.wireEndpointToID_.contains(fanin)) {
            // missing input
            auto func_call_opt = getUnresolvedFunctionCall(circuitName, fanin);
            if (func_call_opt.has_value()) {
                // Generate FUSE circuit for callee first so that its Nodes are available when translating the call
                // std::cout << "caller: " << circuitName << " func call name: " << func_call_opt->name << std::endl;
                processHyCCcircuit(func_call_opt->name);
                // Now, there is definitively a FUSE circuit -> Now translate inputs and outputs of the function call
                translateFunctionCall(circuitName, *func_call_opt);
                assert(context.wireEndpointToID_.contains(fanin));
            } else {
                // input gate is probably in wrong topological order -> translate now
                checkGateInputValuesPresent(circuitName, *fanin.gate);
                translateGate(circuitName, *fanin.gate, fanin.gate->get_operation());
                assert(context.wireEndpointToID_.contains(fanin));
                // wire_endpoint_hasht hasher;
                // std::cout << "[DEBUG] gate op with missing value: " << gate.to_string() << " in circuit: " << circuitName << std::endl;
                // std::cout << "missing fanin operation " << fanin.gate->to_string() << std::endl;
                // throw std::logic_error("Missing input value for wire with hash: " + std::to_string(hasher(fanin)));
            }
        }
    }
}

// For debugging purposes
void HyCCAdapterWithCalls::printGateInputs(const std::string &circuitName, simple_circuitt::gatet &gate) {
    wire_endpoint_hasht hasher;
    // if (gate.get_operation() == simple_circuitt::SPLIT || gate.get_operation() == simple_circuitt::XOR) {
    debugOut << "[" << circuitName << "] ";
    debugOut << gate.to_string() << " : ";
    for (auto fanin : gate.fanin_range()) {
        debugOut << hasher(fanin) << ", ";
    }
    debugOut << " -> ";
    debugOut.flush();
    // }
}
void HyCCAdapterWithCalls::printGateOutputs(const std::string &circuitName, simple_circuitt::gatet &gate) {
    wire_endpoint_hasht hasher;
    if (gate.get_operation() == simple_circuitt::SPLIT) {
        for (unsigned int wire_i = 0; wire_i < gate.get_fanouts().size(); ++wire_i) {
            simple_circuitt::gatet::wire_endpointt wire_endpoint({&gate, wire_i});
            debugOut << hasher(wire_endpoint) << ", ";
        }
    } else if (gate.get_operation() == simple_circuitt::OUTPUT) {
        debugOut << hasher(gate.fanins.at(0));
    } else {
        debugOut << hasher(primary_output(&gate));
    }
    debugOut << std::endl;
}

void HyCCAdapterWithCalls::translateGate(const std::string &circuitName, simple_circuitt::gatet &gate, simple_circuitt::GATE_OP op) {
    // Here, all fanins MUST be present, no matter where they came from
    switch (op) {
        case simple_circuitt::NOT:
        case simple_circuitt::NEG:
            visitUnaryGate(circuitName, gate, op);
            break;
        case simple_circuitt::AND:
        case simple_circuitt::OR:
        case simple_circuitt::XOR:
        case simple_circuitt::ADD:
        case simple_circuitt::SUB:
        case simple_circuitt::MUL:
            visitBinaryGate(circuitName, gate, op);
            break;
        case simple_circuitt::COMBINE:
            visitCombineGate(circuitName, gate);
            break;
        case simple_circuitt::SPLIT:
            visitSplitGate(circuitName, gate);
            break;
        case simple_circuitt::ONE:
        case simple_circuitt::CONST:
            visitConstantGate(circuitName, gate);
            break;
        case simple_circuitt::INPUT:
        case simple_circuitt::OUTPUT:
            // inputs and outputs are handled separately
            break;
        case simple_circuitt::LUT:
            throw std::logic_error("HyCC operation LUT is not supported");
        default:
            throw std::logic_error("Unsupported HyCC Operation " + op);
    }
}

void HyCCAdapterWithCalls::processHyCCcircuit(const std::string &circuitName) {
    simple_circuitt &hyccCirc = hyccCircuits_.at(circuitName);
    HyCCcircuitContext &context = circuitContexts_[circuitName];

    // std::cout << "reached " << circuitName << std::endl;
    // if circuit has been processed already, there is nothing left to do here
    if (moduleBuilder_.containsCircuit(circuitName)) {
        return;
    }
    // Create FUSE CircuitBuilder
    moduleBuilder_.addCircuit(circuitName);

    // translate input nodes
    translateCircuitInputs(circuitName);

    /*
    traverse gates in circuit:
    if all fanins exist -> just add node
    if there are fanins missing -> lookup if they are output from some function call site
        function call: return var -> gates -> fanouts are fanin to some gate?
        if so: translate that function call first
    afterwards, return to translating as usual
     */

    // const auto gate_visitor = [&](simple_circuitt::gatet *gate) {
    for (auto &gate : hyccCirc.gates()) {
        auto op = gate.get_operation();
        if (op == simple_circuitt::INPUT && context.wireEndpointToID_.contains(primary_output(&gate))) {
            // Global input has already been visited
            continue;
        }
        if (op == simple_circuitt::OUTPUT) {
            // Global output will be visited afterwards
            for (auto outputVar : hyccCirc.ordered_outputs()) {
                for (auto outputGate : outputVar->gates) {
                    if (outputGate->fanins.at(0) == gate.fanins.at(0)) {
                        continue;
                    }
                }
            }
        }

        // Check if any input values are missing due to unresolved function calls
        checkGateInputValuesPresent(circuitName, gate);
        translateGate(circuitName, gate, op);
    };
    // hyccCirc.topological_traversal(gate_visitor);
    // translate all gates and function calls together
    translateCircuitOutputs(circuitName);
}

void HyCCAdapterWithCalls::loadCircFiles() {
    // for every circuit file in the directory:
    for (const auto &filename : hyccCircuitFiles) {
        const auto file_path = hyccCircuitDirectory / filename;
        simple_circuitt hycc_circuit(hyccLogger_, "");

        std::ifstream file(file_path);
        if (!file.is_open() || !file.good()) {
            throw hycc_error("could not open file: " + file_path.string());
        }

        hycc_circuit.read(file);
        auto circ_name = hycc_circuit.name();
        hyccCircuits_.emplace(circ_name, std::move(hycc_circuit));
    }
    auto it = hyccCircuits_.find(hyccMainName_);
    if (it == hyccCircuits_.end()) {
        throw hycc_error("Couldn't find main circuit " + hyccMainName_);
    }
    // setup main circuit in FUSE module
    moduleBuilder_.setEntryCircuitName(hyccMainName_);
    // setup context for entry circuit
}

/**
 * @brief Creates a FUSE Module from the HyCC Circuits and saves it to disk.
 * This function preserves the function calls in HyCC and creates different circuits with call nodes for the function call sites in HyCC.
 *
 * @param pathToCmbFile path to a .cmb file that contains all the names of the circuits
 * @param outputBufferPath the path on which the FUSE module will be saved. If left empty, it uses circuitDirectory and creates a .fs file there.
 */
void loadFUSEFromHyCCAndSaveToFile(const std::string &pathToCmbFile, const std::string &outputBufferPath, const std::string &entryCircuitName) {
    HyCCAdapterWithCalls hyccAdapter;
    /*
     * Setup HyCC Adapter
     */

    // all the circuit files must reside within the circuit directory
    // iterate through all files inside directory and add them to circuit_files_
    std::filesystem::path cmbPath(pathToCmbFile);
    hyccAdapter.hyccCircuitDirectory = cmbPath.parent_path();
    std::ifstream cmbContent(cmbPath);

    std::string line;
    while (std::getline(cmbContent, line)) {
        hyccAdapter.hyccCircuitFiles.push_back(line);
    }
    hyccAdapter.hyccMainName_ = entryCircuitName;

    /*
     * Load HyCC Circuit using HyCC Adapter:
     * load the files from memory, then setup FUSE module
     */
    hyccAdapter.loadCircFiles();
    // process main circuit: this starts translating from the main circuit
    hyccAdapter.processHyCCcircuit(hyccAdapter.hyccMainName_);
    // write FUSE to file
    hyccAdapter.writeFUSEToFile(outputBufferPath);
}

/**
 * @brief Creates a FUSE Module from the HyCC Circuits and saves it to disk.
 * This function preserves the function calls in HyCC and creates different circuits with call nodes for the function call sites in HyCC.
 *
 * @param pathToCmbFile path to a .cmb file that contains all the names of the circuits
 * @return core::ModuleContext object that contains the compiled module with translated circuits
 */
core::ModuleContext loadFUSEFromHyCCWithCalls(const std::string &pathToCmbFile, const std::string &entryCircuitName) {
    HyCCAdapterWithCalls hyccAdapter;
    /*
     * Setup HyCC Adapter
     */

    // all the circuit files must reside within the circuit directory
    // iterate through all files inside directory and add them to circuit_files_
    std::filesystem::path cmbPath(pathToCmbFile);
    hyccAdapter.hyccCircuitDirectory = cmbPath.parent_path();
    std::ifstream cmbContent(cmbPath);

    std::string line;
    while (std::getline(cmbContent, line)) {
        hyccAdapter.hyccCircuitFiles.push_back(line);
    }
    hyccAdapter.hyccMainName_ = entryCircuitName;

    /*
     * Load HyCC Circuit using HyCC Adapter:
     * load the files from memory, then setup FUSE module
     */
    hyccAdapter.loadCircFiles();
    // process main circuit: this starts translating from the main circuit
    hyccAdapter.processHyCCcircuit(hyccAdapter.hyccMainName_);
    return hyccAdapter.getFinishedModuleContext();
}

/*
*******************************************************
*******************************************************
* HyCC Adapter for translating with function inlining *
*******************************************************
*******************************************************
*/

class HyCCAdapter {
    using Offset = uint32_t;
    ModuleBuilder module_builder_;
    loggert cbmc_logger_;
    std::unordered_map<simple_circuitt::gatet::wire_endpointt,
                       fuse::frontend::Identifier, wire_endpoint_hasht>
        wire_endpoint_to_identifier_;

    std::unordered_map<simple_circuitt::gatet::wire_endpointt,
                       Offset, wire_endpoint_hasht>
        wire_endpoint_to_offset_;

    void visit_unary_gate(simple_circuitt::gatet *gate,
                          simple_circuitt::GATE_OP op);
    void visit_binary_gate(simple_circuitt::gatet *gate,
                           simple_circuitt::GATE_OP op);
    void visit_combine_gate(simple_circuitt::gatet *gate,
                            simple_circuitt::GATE_OP op);
    void visit_split_gate(simple_circuitt::gatet *gate,
                          simple_circuitt::GATE_OP op);
    void visit_constant_gate(simple_circuitt::gatet *gate,
                             simple_circuitt::GATE_OP op);

    fuse::frontend::Identifier find_identifier_for_wire(
        const simple_circuitt::gatet::wire_endpointt &key);
    Offset find_offset_for_wire(const simple_circuitt::gatet::wire_endpointt &key);
    ir::PrimitiveType get_primitive_type(simple_circuitt::gatet *gate);
    void print_gate_inputs(simple_circuitt::gatet *gate);
    void print_gate_outputs(simple_circuitt::gatet *gate);
    void print_split_gate(simple_circuitt::gatet *gate);

   public:
    std::filesystem::path circuit_directory_;
    std::vector<std::string> circuit_files_;
    std::unordered_map<std::string, simple_circuitt> cbmc_circuits_;

    simple_circuitt &load_circuit_files();
    void process_circuit_inputs(const simple_circuitt &circuit);
    void process_circuit_outputs(const simple_circuitt &circuit);
    void topological_traversal(simple_circuitt &circuit);
    void writeFUSEToFile(const std::string &pathToSaveBuffer);
    void cleanupHyCC() {
        for (auto circ : cbmc_circuits_) {
            circ.second.cleanup();
        }
    }
};

void loadFUSEFromHyCC(const std::string &circuitDirectory,
                      const std::string &outputBufferPath) {
    HyCCAdapter hyccAdapter;

    /*
     * all the circuit files must reside within the circuit directory
     * iterate through all files inside directory and add them to circuit_files_
     */
    std::filesystem::path circuitDirectoryPath(circuitDirectory);
    hyccAdapter.circuit_directory_ = circuitDirectoryPath;

    // add all .circ files to the list of circuit files
    for (const auto &entry :
         std::filesystem::directory_iterator(circuitDirectory)) {
        if (entry.exists() && !entry.path().empty() &&
            entry.path().extension() == ".circ") {
            hyccAdapter.circuit_files_.push_back(
                entry.path().filename().string());
        }
    }

    // load all files, process inputs, then gate, and then outputs
    auto mainCircuit = hyccAdapter.load_circuit_files();
    hyccAdapter.process_circuit_inputs(mainCircuit);
    hyccAdapter.topological_traversal(mainCircuit);
    hyccAdapter.process_circuit_outputs(mainCircuit);
    hyccAdapter.writeFUSEToFile(outputBufferPath);
}

/*
****************************************
HyCC Adapter Implementations
****************************************
*/

simple_circuitt &HyCCAdapter::load_circuit_files() {
    // for every circuit file in the directory:
    for (const auto &filename : circuit_files_) {
        const auto file_path = circuit_directory_ / filename;
        simple_circuitt cbmc_circuit(cbmc_logger_, "");

        std::ifstream file(file_path);
        if (!file.is_open() || !file.good()) {
            throw hycc_error("could not open file: " + file_path.string());
        }

        cbmc_circuit.read(file);
        auto circuit_name = cbmc_circuit.name();
        cbmc_circuits_.emplace(circuit_name, std::move(cbmc_circuit));
    }
    const std::string main_name = "mpc_main";
    module_builder_.addCircuit(main_name);
    module_builder_.setEntryCircuitName(main_name);
    auto it = cbmc_circuits_.find(main_name);
    if (it == cbmc_circuits_.end()) {
        throw hycc_error("Couldn't find main circuit " + main_name);
    }
    auto &main_circuit = it->second;
    // This lets HyCC Link all the circuits together.
    // This means that for every callsite, the call arguments and return values of the callee are linked
    // to the gates from the caller that provides the argument values / consumes the return values.
    // When translating this to FUSE using topological traversal, this effectively means function inlining,
    // such that the calls are not preserved but the function gates are inlined at every call site.
    main_circuit.link(cbmc_circuits_);
    return main_circuit;
}

void HyCCAdapter::writeFUSEToFile(const std::string &pathToSaveBuffer) {
    module_builder_.finishAndWriteToFile(pathToSaveBuffer);
}

void HyCCAdapter::process_circuit_inputs(const simple_circuitt &circuit) {
    CircuitBuilder *circuitBuilder = module_builder_.getMainCircuit();

    // ordered inputs are of type variablet*
    for (const auto *input_var : circuit.ordered_inputs()) {
        const auto &label = input_var->name;

        // owner is either: { input_alice, input_bob, output }
        const auto input_owner = input_var->owner;

        std::string input_annotation;
        ir::SecurityLevel securityLevel = ir::SecurityLevel::Secure;
        switch (input_owner) {
            case variable_ownert::input_alice: {
                input_annotation = "owner:1";
                break;
            }
            case variable_ownert::input_bob: {
                input_annotation = "owner:2";
                break;
            }
            case variable_ownert::output: {
                input_annotation = label;
                securityLevel = ir::SecurityLevel::Plaintext;
                break;
            }
            default: {
                throw hycc_error("Unknown HyCC Owner Type");
                break;
            }
        }

        // get gates for input variable
        const auto num_gates = input_var->gates.size();
        for (std::size_t i = 0; i < num_gates; ++i) {
            // get current gate that is part of the variable
            auto current_gate = input_var->gates.at(i);
            size_t datatype_offset = circuitBuilder->addDataType(get_primitive_type(current_gate), securityLevel, {}, input_annotation);
            // get wire_endpoint that defines the output for this variable
            auto gate_output = primary_output(current_gate);
            wire_endpoint_to_identifier_[gate_output] = circuitBuilder->addInputNode({datatype_offset}, input_annotation);
        }
    }
}

void HyCCAdapter::process_circuit_outputs(const simple_circuitt &circuit) {
    CircuitBuilder *circuitBuilder = module_builder_.getMainCircuit();
    for (const auto *output_var : circuit.ordered_outputs()) {
        const auto &label = output_var->name;
        auto &gates = output_var->gates;

        const auto num_gates = output_var->gates.size();

        // output var is split into these gates
        // -> just create gates for each output gate
        for (std::size_t i = 0; i < num_gates; ++i) {
            std::vector<uint64_t> datatype_offsets;
            auto current_gate = gates.at(i);
            auto input_key = current_gate->fanins.at(0);
            auto input_node = find_identifier_for_wire(input_key);
            for (auto fanin : current_gate->fanins) {
                size_t datatype_offset = circuitBuilder->addDataType(get_primitive_type(fanin.gate), ir::SecurityLevel::Plaintext);
                datatype_offsets.push_back(datatype_offset);
            }
            // don't need to save them into wire_endpoint_to_identifier_map_
            // as output nodes are not referred by anything inside here anymore
            circuitBuilder->addOutputNode(datatype_offsets, {input_node});
        }
    }
}

fuse::frontend::Identifier HyCCAdapter::find_identifier_for_wire(
    const simple_circuitt::gatet::wire_endpointt &key) {
    auto iterator = wire_endpoint_to_identifier_.find(key);
    if (iterator == wire_endpoint_to_identifier_.end()) {
        throw hycc_error("cannot find node for HyCC wire with label: " + key.gate->label);
    } else {
        return iterator->second;
    }
}

HyCCAdapter::Offset HyCCAdapter::find_offset_for_wire(const simple_circuitt::gatet::wire_endpointt &key) {
    auto iterator = wire_endpoint_to_offset_.find(key);
    if (iterator == wire_endpoint_to_offset_.end()) {
        return 0;
    } else {
        return iterator->second;
    }
}

ir::PrimitiveType HyCCAdapter::get_primitive_type(simple_circuitt::gatet *gate) {
    if (gate->get_width() == 1) {
        return ir::PrimitiveType::Bool;
    } else if (gate->get_width() <= 8) {
        return ir::PrimitiveType::UInt8;
    } else if (gate->get_width() <= 16) {
        return ir::PrimitiveType::UInt16;
    } else if (gate->get_width() <= 32) {
        return ir::PrimitiveType::UInt32;
    } else if (gate->get_width() <= 64) {
        return ir::PrimitiveType::UInt64;
    } else {
        throw hycc_error("unexpected bit width: " + gate->get_width());
    }
}

void HyCCAdapter::visit_unary_gate(simple_circuitt::gatet *gate,
                                   simple_circuitt::GATE_OP op) {
    // unary gate expects exactly two inputs
    if (gate->num_fanins() != 1) {
        throw hycc_error("unary gate received unexpected number of inputs: " + gate->num_fanins());
    }

    // get input and output keys for the wire_endpoint_to_identifier_ map
    auto &key_input = gate->fanin_range()[0];
    auto key_output = primary_output(gate);

    Identifier id_input = find_identifier_for_wire(key_input);
    Offset offset = find_offset_for_wire(key_input);
    Identifier id_output;

    // generate the FUSE node with the correct operation
    switch (op) {
        case simple_circuitt::NOT: {
            id_output = module_builder_.getMainCircuit()->addNode(ir::PrimitiveOperation::Not, {id_input}, {offset});
            break;
        }
        case simple_circuitt::NEG: {
            id_output = module_builder_.getMainCircuit()->addNode(ir::PrimitiveOperation::Neg, {id_input}, {offset});
            break;
        }
        default:
            throw std::logic_error("expected unary operation, unexpected operation " + op);
    }

    // store node ID to wire map
    wire_endpoint_to_identifier_[key_output] = id_output;
}

void HyCCAdapter::visit_binary_gate(simple_circuitt::gatet *gate,
                                    simple_circuitt::GATE_OP op) {
    // binary gate expects exactly two inputs
    if (gate->num_fanins() != 2) {
        throw hycc_error(
            "binary gate received unexpected number of inputs: " + gate->num_fanins());
    }

    // get input and output keys for the wire_endpoint_to_identifier_ map
    auto &key_a = gate->fanin_range()[0];
    auto &key_b = gate->fanin_range()[1];
    auto key_output = primary_output(gate);

    Identifier id_a = find_identifier_for_wire(key_a);
    Identifier id_b = find_identifier_for_wire(key_b);
    Offset offset_a = find_offset_for_wire(key_a);
    Offset offset_b = find_offset_for_wire(key_b);
    Identifier id_output;

    // generate the FUSE node with the correct operation
    switch (op) {
        case simple_circuitt::AND: {
            id_output = module_builder_.getMainCircuit()->addNode(ir::PrimitiveOperation::And, {id_a, id_b}, {offset_a, offset_b});
            break;
        }
        case simple_circuitt::XOR: {
            id_output = module_builder_.getMainCircuit()->addNode(ir::PrimitiveOperation::Xor, {id_a, id_b}, {offset_a, offset_b});
            break;
        }
        case simple_circuitt::OR: {
            id_output = module_builder_.getMainCircuit()->addNode(ir::PrimitiveOperation::Or, {id_a, id_b}, {offset_a, offset_b});
            break;
        }
        case simple_circuitt::ADD: {
            id_output = module_builder_.getMainCircuit()->addNode(ir::PrimitiveOperation::Add, {id_a, id_b}, {offset_a, offset_b});
            break;
        }
        case simple_circuitt::SUB: {
            id_output = module_builder_.getMainCircuit()->addNode(ir::PrimitiveOperation::Sub, {id_a, id_b}, {offset_a, offset_b});
            break;
        }
        case simple_circuitt::MUL: {
            id_output = module_builder_.getMainCircuit()->addNode(ir::PrimitiveOperation::Mul, {id_a, id_b}, {offset_a, offset_b});
            break;
        }
        default:
            throw std::logic_error("expected binary operation, unexpected unary operation " + op);
    }

    // store node ID to wire map
    wire_endpoint_to_identifier_[key_output] = id_output;
}

/*
 * Combine Gate : combine wires of Bits to one Numeral Value
 */
void HyCCAdapter::visit_combine_gate(simple_circuitt::gatet *gate,
                                     simple_circuitt::GATE_OP op) {
    // get output key for the wire_endpoint_to_identifier_ map
    auto key_output = primary_output(gate);
    Identifier id_output;
    CircuitBuilder *circuitBuilder = module_builder_.getMainCircuit();

    // gather all input IDs for combine gate
    std::vector<Identifier> input_nodes;
    std::vector<Offset> input_offsets;
    for (auto &fanin : gate->fanin_range()) {
        input_nodes.push_back(find_identifier_for_wire(fanin));
        input_offsets.push_back(find_offset_for_wire(fanin));
    }

    // generate the FUSE node with a custom operation and correct custom operation name
    id_output = module_builder_.getMainCircuit()->addNode(ir::PrimitiveOperation::Merge, input_nodes);

    // store node ID to wire map
    wire_endpoint_to_identifier_[key_output] = id_output;
}

/*
 * Split Gate : Split one numeral value to Bit wires
 */
void HyCCAdapter::visit_split_gate(simple_circuitt::gatet *gate,
                                   simple_circuitt::GATE_OP op) {
    // split gate splits exactly one input value
    if (gate->num_fanins() != 1) {
        throw hycc_error("split gate received unexpected number of inputs: " + gate->num_fanins());
    }

    // get input and output keys for the wire_endpoint_to_identifier_ map
    auto &key_input = gate->fanin_range()[0];
    Identifier id_input = find_identifier_for_wire(key_input);
    Identifier id_output = module_builder_.getMainCircuit()->addNode(ir::PrimitiveOperation::Split, {id_input});

    // split always splits to boolean gates according to their code
    // see https://gitlab.com/securityengineering/HyCC/-/blob/master/src/libcircuit/simple_circuit.cpp#L355
    assert(gate->get_fanouts().size() > 1);
    for (unsigned int wire_i = 0; wire_i < gate->get_fanouts().size(); ++wire_i) {
        simple_circuitt::gatet::wire_endpointt wire_endpoint({gate, wire_i});
        wire_endpoint_to_identifier_[wire_endpoint] = id_output;
        wire_endpoint_to_offset_[wire_endpoint] = wire_i;
    }
}

void HyCCAdapter::visit_constant_gate(simple_circuitt::gatet *gate,
                                      simple_circuitt::GATE_OP op) {
    auto key_output = primary_output(gate);
    Identifier id_output;

    // generate the FUSE node with a constant value
    // ONE is meant to be a boolean gate, which is why the value "true" will be saved
    switch (op) {
        case simple_circuitt::ONE: {
            id_output = module_builder_.getMainCircuit()->addConstantNodeWithPayload(true);
            break;
        }
        case simple_circuitt::CONST: {
            id_output = module_builder_.getMainCircuit()->addConstantNodeWithPayload(gate->get_value());
            break;
        }
        default:
            throw std::logic_error("expected combine or split operation, unexpected operation " + op);
    }

    // store node ID to wire map
    wire_endpoint_to_identifier_[key_output] = id_output;
}

void HyCCAdapter::print_gate_inputs(simple_circuitt::gatet *gate) {
    wire_endpoint_hasht hasher;
    if (gate->get_operation() == simple_circuitt::SPLIT || gate->get_operation() == simple_circuitt::XOR) {
        std::cout << gate->to_string() << " : ";
        for (auto fanin : gate->fanin_range()) {
            std::cout << hasher(fanin) << ", ";
        }
        std::cout << " -> ";
        std::cout.flush();
    }
}

void HyCCAdapter::print_gate_outputs(simple_circuitt::gatet *gate) {
    wire_endpoint_hasht hasher;
    if (gate->get_operation() == simple_circuitt::SPLIT) {
        for (unsigned int wire_i = 0; wire_i < gate->get_fanouts().size(); ++wire_i) {
            simple_circuitt::gatet::wire_endpointt wire_endpoint({gate, wire_i});
            std::cout << hasher(wire_endpoint) << ", ";
        }
    } else if (gate->get_operation() == simple_circuitt::OUTPUT) {
        std::cout << hasher(gate->fanins.at(0));
    } else {
        std::cout << hasher(primary_output(gate));
    }
    std::cout << std::endl;
}

void HyCCAdapter::topological_traversal(simple_circuitt &circuit) {
    const auto visitor = [this](simple_circuitt::gatet *gate) {
        // print_gate_inputs(gate);
        auto gate_op = gate->get_operation();
        switch (gate_op) {
            case simple_circuitt::NOT:
            case simple_circuitt::NEG:
                visit_unary_gate(gate, gate_op);
                break;
            case simple_circuitt::AND:
            case simple_circuitt::OR:
            case simple_circuitt::XOR:
            case simple_circuitt::ADD:
            case simple_circuitt::SUB:
            case simple_circuitt::MUL:
                visit_binary_gate(gate, gate_op);
                break;
            case simple_circuitt::COMBINE:
                visit_combine_gate(gate, gate_op);
                break;
            case simple_circuitt::SPLIT:
                visit_split_gate(gate, gate_op);
                break;
            case simple_circuitt::ONE:
            case simple_circuitt::CONST:
                visit_constant_gate(gate, gate_op);
                break;
            case simple_circuitt::INPUT:
            case simple_circuitt::OUTPUT:
                // inputs and outputs are handled separately
                break;
            case simple_circuitt::LUT:
                throw hycc_error("HyCC operation LUT is not supported");
        };
        // print_gate_outputs(gate);
    };
    circuit.topological_traversal(visitor);
}

}  // namespace fuse::frontend
