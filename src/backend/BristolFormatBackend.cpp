#include "BristolFormatBackend.h"

#include <unordered_map>

namespace fuse::backend {

class BristolFormatGenerator {
    using Identifier = uint64_t;

   private:
    std::stringstream bristol_;
    Identifier currentWireNum_ = 0;
    std::unordered_map<Identifier, Identifier> nodeToWire;

    template <core::ir::PrimitiveOperation op>
    void visitNode(const core::NodeReadOnly& node);
    void visitNode(const core::NodeReadOnly& node);
    void visitNode(const core::NodeReadOnly& node, const core::ModuleReadOnly& parentModule);
    void visitCircuit(const core::CircuitReadOnly& circuit, const core::ModuleReadOnly& parentModule);

   public:
    std::string generateBristolFormat(const core::CircuitReadOnly& circuit);
    std::string generateBristolFormat(const core::ModuleReadOnly& module);
};

template <>
void BristolFormatGenerator::visitNode<core::ir::PrimitiveOperation::And>(const core::NodeReadOnly& node) {
    auto inputs = node.getInputNodeIDs();
    auto id = node.getNodeID();
    auto wire = currentWireNum_++;
    nodeToWire[id] = wire;
    bristol_ << "2 1 " << nodeToWire[inputs[0]] << " " << nodeToWire[inputs[1]] << " " << wire << " AND\n";
}
template <>
void BristolFormatGenerator::visitNode<core::ir::PrimitiveOperation::Xor>(const core::NodeReadOnly& node) {
    auto inputs = node.getInputNodeIDs();
    auto id = node.getNodeID();
    auto wire = currentWireNum_++;
    nodeToWire[id] = wire;
    bristol_ << "2 1 " << nodeToWire[inputs[0]] << " " << nodeToWire[inputs[1]] << " " << wire << " XOR\n";
}
template <>
void BristolFormatGenerator::visitNode<core::ir::PrimitiveOperation::Not>(const core::NodeReadOnly& node) {
    auto inputs = node.getInputNodeIDs();
    auto id = node.getNodeID();
    auto wire = currentWireNum_++;
    nodeToWire[id] = wire;
    bristol_ << "1 1 " << nodeToWire[inputs[0]] << " " << wire << " INV\n";
}
template <>
void BristolFormatGenerator::visitNode<core::ir::PrimitiveOperation::Or>(const core::NodeReadOnly& node) {
    // A OR B = NOT(NOT A AND NOT B)
    auto inputs = node.getInputNodeIDs();
    auto notA = currentWireNum_++;
    bristol_ << "1 1 " << nodeToWire[inputs[0]] << " " << notA << " INV\n";
    auto notB = currentWireNum_++;
    bristol_ << "1 1 " << nodeToWire[inputs[1]] << " " << notB << " INV\n";
    auto andWire = currentWireNum_++;
    bristol_ << "2 1 " << notA << " " << notB << " " << andWire << " AND\n";
    auto resWire = currentWireNum_++;
    bristol_ << "1 1 " << andWire << " " << resWire << " INV\n";
    nodeToWire[node.getNodeID()] = resWire;
}
template <>
void BristolFormatGenerator::visitNode<core::ir::PrimitiveOperation::Nand>(const core::NodeReadOnly& node) {
    // A NAND B = NOT(A AND B)
    auto inputs = node.getInputNodeIDs();
    auto andWire = currentWireNum_++;
    bristol_ << "2 1 " << nodeToWire[inputs[0]] << " " << nodeToWire[inputs[1]] << " " << andWire << " AND\n";
    auto resWire = currentWireNum_++;
    bristol_ << "1 1 " << andWire << " " << resWire << " INV\n";
    nodeToWire[node.getNodeID()] = resWire;
}
template <>
void BristolFormatGenerator::visitNode<core::ir::PrimitiveOperation::Nor>(const core::NodeReadOnly& node) {
    // A NOR B = (NOT A) AND (NOT B)
    auto inputs = node.getInputNodeIDs();
    auto notA = currentWireNum_++;
    bristol_ << "1 1 " << nodeToWire[inputs[0]] << " " << notA << " INV\n";
    auto notB = currentWireNum_++;
    bristol_ << "1 1 " << nodeToWire[inputs[1]] << " " << notB << " INV\n";
    auto andWire = currentWireNum_++;
    bristol_ << "2 1 " << notA << " " << notB << " " << andWire << " AND\n";
    nodeToWire[node.getNodeID()] = andWire;
}
template <>
void BristolFormatGenerator::visitNode<core::ir::PrimitiveOperation::Xnor>(const core::NodeReadOnly& node) {
    // A XNOR B = NOT(A XOR B)
    auto inputs = node.getInputNodeIDs();
    auto xorWire = currentWireNum_++;
    bristol_ << "2 1 " << nodeToWire[inputs[0]] << " " << nodeToWire[inputs[1]] << " " << xorWire << " XOR\n";
    auto resWire = currentWireNum_++;
    bristol_ << "1 1 " << xorWire << " " << resWire << " INV\n";
    nodeToWire[node.getNodeID()] = resWire;
}
template <>
void BristolFormatGenerator::visitNode<core::ir::PrimitiveOperation::Constant>(const core::NodeReadOnly& node) {
    bool val = node.getConstantBool();
    if (val) {
        // true = A XOR (NOT A)
        auto prevWire = currentWireNum_ - 1;
        auto notWire = currentWireNum_++;
        bristol_ << "1 1 " << prevWire << " " << prevWire << " INV\n";
        auto xorWire = currentWireNum_++;
        bristol_ << "2 1 " << prevWire << " " << notWire << " " << xorWire << " XOR\n";
        nodeToWire[node.getNodeID()] = xorWire;
    } else {
        // false = A XOR A
        auto prevWire = currentWireNum_ - 1;
        auto xorWire = currentWireNum_++;
        bristol_ << "2 1 " << prevWire << " " << prevWire << " " << xorWire << " XOR\n";
        nodeToWire[node.getNodeID()] = xorWire;
    }
}

template <core::ir::PrimitiveOperation op>
void BristolFormatGenerator::visitNode(const core::NodeReadOnly& node) {
    std::string opName = node.getOperationName();
    throw std::runtime_error("Cannot translate node with operation: " + opName);
}

void BristolFormatGenerator::visitNode(const core::NodeReadOnly& node) {
    using op = core::ir::PrimitiveOperation;
    auto inputs = node.getInputNodeIDs();
    auto id = node.getNodeID();
    switch (node.getOperation()) {
        case op::Input:
            return;
        case op::Output: {
            auto in = node.getInputNodeIDs()[0];
            nodeToWire[node.getNodeID()] = nodeToWire[in];
            return;
        }
        case op::And:
            visitNode<op::And>(node);
            return;
        case op::Xor:
            visitNode<op::Xor>(node);
            return;
        case op::Not:
            visitNode<op::Not>(node);
            return;
        case op::Or:
            visitNode<op::Or>(node);
            return;
        case op::Nand:
            visitNode<op::Nand>(node);
            return;
        case op::Nor:
            visitNode<op::Nor>(node);
            return;
        case op::Xnor:
            visitNode<op::Xnor>(node);
            return;
        case op::Constant:
            visitNode<op::Constant>(node);
            return;
        default:
            std::string opName = node.getOperationName();
            throw std::runtime_error("Cannot translate node with operation: " + opName);
    }
}

/** @brief Translates *Boolean* Circuit to Bristol.
 *
 * Bristol Format:
 * line 1: [number of gates] [number of wires]
 * line 2: [number of input wires party 1] [number of input wires party 2] [number of output wires]
 * blank line
 * for each gate:
 *      num_in_wires 1 [list_in_wires] out_wire op - XOR, AND, INV
 */
std::string BristolFormatGenerator::generateBristolFormat(const core::CircuitReadOnly& circuit) {
    for (auto input_id : circuit.getInputNodeIDs()) {
        nodeToWire[input_id] = currentWireNum_++;
    }
    circuit.topologicalTraversal([&, this](const core::NodeReadOnly& node) { visitNode(node); });
    std::stringstream header;
    auto numInputs = circuit.getNumberOfInputs();
    auto numParty1 = numInputs / 2;
    auto numParty2 = numInputs - numParty1;
    auto numOutputs = circuit.getNumberOfOutputs();
    auto numWires = currentWireNum_;
    auto numGates = numWires - (numInputs + numOutputs);
    header << numGates << " " << numWires;
    header << numParty1 << " " << numParty2 << " " << numOutputs << "\n";
    return header.str() + bristol_.str();
}

std::string generateBristolFormatFrom(const core::CircuitReadOnly& circuit) {
    BristolFormatGenerator gen;
    return gen.generateBristolFormat(circuit);
}

void BristolFormatGenerator::visitNode(const core::NodeReadOnly& node, const core::ModuleReadOnly& parentModule) {
    if (node.isSubcircuitNode()) {
        auto subcircuit = parentModule.getCircuitWithName(node.getSubCircuitName());
        visitCircuit(*subcircuit, parentModule);
    } else {
        visitNode(node);
    }
}

void BristolFormatGenerator::visitCircuit(const core::CircuitReadOnly& circuit, const core::ModuleReadOnly& parentModule) {
    circuit.topologicalTraversal([&, this](const core::NodeReadOnly& node) { visitNode(node, parentModule); });
}

std::string BristolFormatGenerator::generateBristolFormat(const core::ModuleReadOnly& module) {
    auto entryCircuit = module.getEntryCircuit();
    for (auto input_id : entryCircuit->getInputNodeIDs()) {
        nodeToWire[input_id] = currentWireNum_++;
    }
    visitCircuit(*entryCircuit, module);

    std::stringstream header;
    auto numInputs = entryCircuit->getNumberOfInputs();
    auto numParty1 = numInputs / 2;
    auto numParty2 = numInputs - numParty1;
    auto numOutputs = entryCircuit->getNumberOfOutputs();
    auto numWires = currentWireNum_;
    auto numGates = numWires - (numInputs + numOutputs);
    header << numGates << " " << numWires;
    header << numParty1 << " " << numParty2 << " " << numOutputs << "\n";
    return header.str() + bristol_.str();
}

std::string generateBristolFormatFrom(const core::ModuleReadOnly& module) {
    BristolFormatGenerator gen;
    return gen.generateBristolFormat(module);
}

}  // namespace fuse::backend
