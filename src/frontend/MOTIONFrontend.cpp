#include "MOTIONFrontend.h"

#include "ModuleBuilder.h"

// MOTION
#include "protocols/arithmetic_gmw/arithmetic_gmw_gate.h"
#include "protocols/astra/astra_gate.h"
#include "protocols/bmr/bmr_gate.h"
#include "protocols/boolean_gmw/boolean_gmw_gate.h"
#include "protocols/constant/constant_gate.h"
#include "protocols/conversion/b2a_gate.h"
#include "protocols/conversion/conversion_gate.h"
#include "protocols/data_management/simdify_gate.h"
#include "protocols/data_management/subset_gate.h"
#include "protocols/data_management/unsimdify_gate.h"
#include "protocols/share_wrapper.h"

namespace fuse::frontend {

/*
    Arithmetic GMW: number of input values == number of SIMD values
         each wire is of unsigned int type with 8, 16, 32, 64, 128 bits
    similar for BMR and Boolean GMW
    --> Wire ==> SIMD values

    Arithmetic GMW input gate: one output wire

    BMR + Boolean GMW input gate:
    - bit size + number of SIMD
    --> each bit has the same amount of SIMD values
    --> each bit is one output wire with the same amount of SIMD values inside that wire

    store NUMBER of SIMD values only for inputs

    Arithmetic GMW
        - one single parent wire
        - but wire can hold SIMD values

    BMR and Boolean GMW
        - multiple parent wires
        - each of which can hold SIMD values

    Wenn ich das richtig verstehe, steht ein Wire für einen Wert (z.B. bei nem AND für das eine Bit)
        - für parallele Berechnung kann dieses eine Wire mehrere SIMD Werte enthalten, die für diesen einen Wert stehen.

    - wie man SIMD wires anders verwenden kann
        - extra gates: neues wire mit nur einem subset / einzelner wert
        - alle metainfos für viele wires in einem

    - mehrere wires in einem share
        - gleiche repräsentation für mehrere sachen verwenden kann
        - z.B. gegeben Integer, speichern als mehrere Bits
        - in wires mehrere integer parallel -> zu anderem protokoll konvertieren
        - dann zu anderem protokoll konvertieren (aGMW)

    - helper klassen: layer
        - OT objekte, gate objekte
        - share wrapper: plus -> additionsgate aber nur wenn protokoll das unterstützt
        - secure uint: kann alles was unsigned integer machen kann, + und egal was für protokoll --> shares
        - share: boolean gmw -> bitstring im share, arith -> ein wire mit einem integer wert
            -> konvertieren: mehrere wires --> konkatenieren (anders interpretieren)
    */

namespace mo = encrypto::motion;

core::ir::PrimitiveType bitlenToType(size_t bitlen) {
    switch (bitlen) {
        case 1:
            return core::ir::PrimitiveType::Bool;
        case 8:
            return core::ir::PrimitiveType::UInt8;
        case 16:
            return core::ir::PrimitiveType::UInt16;
        case 32:
            return core::ir::PrimitiveType::UInt32;
        case 64:
            return core::ir::PrimitiveType::UInt64;
        default:
            throw std::runtime_error("Illegal Bitlength: " + std::to_string(bitlen));
    }
}

class MOTIONFrontendAdapter {
    friend class mo::Gate;

    using Identifier = uint64_t;
    using MotionID = uint64_t;
    using Offset = uint32_t;
    using SimdifyGatePointer = std::shared_ptr<mo::SimdifyGate>;
    using UnsimdifyGatePointer = std::shared_ptr<mo::UnsimdifyGate>;
    using SubsetGatePointer = std::shared_ptr<mo::SubsetGate>;
    using OneGatePointer = std::shared_ptr<mo::OneGate>;
    using TwoGatePointer = std::shared_ptr<mo::TwoGate>;
    using ThreeGatePointer = std::shared_ptr<mo::ThreeGate>;
    using Op = fuse::core::ir::PrimitiveOperation;

   private:
    size_t secBool;
    size_t ptBool;
    size_t secBool8;
    size_t ptBool8;
    size_t secBool16;
    size_t ptBool16;
    size_t secBool32;
    size_t ptBool32;
    size_t secBool64;
    size_t ptBool64;
    size_t secUint8;
    size_t secUint16;
    size_t secUint32;
    size_t secUint64;
    size_t ptUint8;
    size_t ptUint16;
    size_t ptUint32;
    size_t ptUint64;

    std::unordered_map<MotionID, Identifier> motionWireToFuseNode;
    std::unordered_map<MotionID, Offset> motionWireToFuseOffset;

    // Utility
    size_t bitlenToSecType(size_t bitlen) const;

    // Translation of MOTION Gates
    void translateUnaryOperation(frontend::CircuitBuilder& cb, OneGatePointer gate, fuse::core::ir::PrimitiveOperation op);
    void translateBinaryOperation(frontend::CircuitBuilder& cb, TwoGatePointer gate, fuse::core::ir::PrimitiveOperation op, const std::string& annotations = "");
    void translateTernaryOperation(frontend::CircuitBuilder& cb, ThreeGatePointer gate, fuse::core::ir::PrimitiveOperation op);

    void translateInputGate(frontend::CircuitBuilder& cb, mo::InputGatePointer gate);
    void translateOutputGate(frontend::CircuitBuilder& cb, mo::OutputGatePointer gate);
    void translateArithmeticGMWGate(frontend::CircuitBuilder& cb, encrypto::motion::GatePointer gate);
    void translateAstraGate(frontend::CircuitBuilder& cb, encrypto::motion::GatePointer gate);
    void translateBMRGate(frontend::CircuitBuilder& cb, encrypto::motion::GatePointer gate);
    void translateBooleanGMWGate(frontend::CircuitBuilder& cb, encrypto::motion::GatePointer gate);
    void translateConstantBooleanGate(frontend::CircuitBuilder& cb, encrypto::motion::GatePointer gate);
    void translateConstantArithmeticGate(frontend::CircuitBuilder& cb, encrypto::motion::GatePointer gate);
    void translateConversionGate(frontend::CircuitBuilder& cb, encrypto::motion::GatePointer gate);
    void translateSimdifyGate(frontend::CircuitBuilder& cb, SimdifyGatePointer gate);
    void translateUnsimdifyGate(frontend::CircuitBuilder& cb, UnsimdifyGatePointer gate);
    void translateSubsetGate(frontend::CircuitBuilder& cb, SubsetGatePointer gate);

   public:
    void initDataTypes(frontend::CircuitBuilder& cb);
    void translateMOTIONGate(frontend::CircuitBuilder& cb, encrypto::motion::GatePointer gate);
};

size_t MOTIONFrontendAdapter::bitlenToSecType(size_t bitlen) const {
    switch (bitlen) {
        case 1:
            return secBool;
        case 8:
            return secUint8;
        case 16:
            return secUint16;
        case 32:
            return secUint32;
        case 64:
            return secUint64;
        default:
            throw std::runtime_error("Illegal bitlength: " + std::to_string(bitlen));
    }
}

void MOTIONFrontendAdapter::translateTernaryOperation(frontend::CircuitBuilder& cb, ThreeGatePointer gate, fuse::core::ir::PrimitiveOperation op) {
    std::vector<Identifier> inputs;
    std::vector<Offset> offsets;

    // a ? b : c
    auto a = gate->GetParentA();
    auto b = gate->GetParentB();
    auto c = gate->GetParentC();
    auto outs = gate->GetOutputWires();
    assert(b.size() == c.size());     // same amount of input wires
    assert(c.size() == outs.size());  // same number of input as output wires

    // gather all input wires from all parents
    for (auto& in : a) {
        auto node = motionWireToFuseNode[in->GetWireId()];
        auto offset = motionWireToFuseOffset[in->GetWireId()];
        inputs.push_back(node);
        offsets.push_back(offset);
    }
    for (auto& in : b) {
        auto node = motionWireToFuseNode[in->GetWireId()];
        auto offset = motionWireToFuseOffset[in->GetWireId()];
        inputs.push_back(node);
        offsets.push_back(offset);
    }
    for (auto& in : c) {
        auto node = motionWireToFuseNode[in->GetWireId()];
        auto offset = motionWireToFuseOffset[in->GetWireId()];
        inputs.push_back(node);
        offsets.push_back(offset);
    }

    std::string size_annotation = "cond:" + std::to_string(a.size()) + ",val:" + std::to_string(b.size());
    auto n = cb.addNodeWithNumberOfOutputs(op, inputs, offsets, outs.size(), size_annotation);

    for (int i = 0; i < outs.size(); ++i) {
        auto out = outs.at(i)->GetWireId();
        motionWireToFuseNode[out] = n;
        motionWireToFuseOffset[out] = i;
    }
}

void MOTIONFrontendAdapter::translateBinaryOperation(frontend::CircuitBuilder& cb, TwoGatePointer gate, fuse::core::ir::PrimitiveOperation op, const std::string& annotations) {
    std::vector<Identifier> inputs;
    std::vector<Offset> offsets;

    auto a = gate->GetParentA();
    auto b = gate->GetParentB();
    auto outs = gate->GetOutputWires();
    assert(a.size() == b.size());     // same amount of input values
    assert(a.size() == outs.size());  // same number of input as output values

    // gather all input wires from all parents
    for (auto& in : a) {
        auto node = motionWireToFuseNode[in->GetWireId()];
        auto offset = motionWireToFuseOffset[in->GetWireId()];
        inputs.push_back(node);
        offsets.push_back(offset);
    }
    for (auto& in : b) {
        auto node = motionWireToFuseNode[in->GetWireId()];
        auto offset = motionWireToFuseOffset[in->GetWireId()];
        inputs.push_back(node);
        offsets.push_back(offset);
    }

    auto n = cb.addNodeWithNumberOfOutputs(op, inputs, offsets, outs.size());
    for (int i = 0; i < outs.size(); ++i) {
        auto out = outs.at(i)->GetWireId();
        motionWireToFuseNode[out] = n;
        motionWireToFuseOffset[out] = i;
    }
}

void MOTIONFrontendAdapter::translateUnaryOperation(frontend::CircuitBuilder& cb, OneGatePointer gate, fuse::core::ir::PrimitiveOperation op) {
    std::vector<Identifier> inputs;
    std::vector<Offset> offsets;

    auto a = gate->GetParent();
    auto outs = gate->GetOutputWires();
    assert(a.size() == outs.size());  // same number of input as output wires

    // gather all inputs to the node
    for (size_t val = 0; val < a.size(); ++val) {
        auto in1 = a.at(val)->GetWireId();
        auto node1 = motionWireToFuseNode[in1];
        auto offset1 = motionWireToFuseOffset[in1];
        inputs.push_back(node1);
        offsets.push_back(offset1);
    }

    auto n = cb.addNodeWithNumberOfOutputs(op, inputs, offsets, outs.size());

    for (int i = 0; i < outs.size(); ++i) {
        auto out = outs.at(i)->GetWireId();
        motionWireToFuseNode[out] = n;
        motionWireToFuseOffset[out] = i;
    }
}

void MOTIONFrontendAdapter::translateSimdifyGate(frontend::CircuitBuilder& cb, SimdifyGatePointer gate) {
    // input: multiple wires to be simdified from multiple parent shares
    // --> total number of input wires = number of parents * input wires per parent
    // [ {parent 0: in1, in2, in3} | {parent 1: in1, in2, in3} | {parent 2: in1, in2, in3} | ... ]
    // --> total number of output wires = input wires per parent
    // [ out0 | out1 | out2 | ... ]
    std::vector<Identifier> inputs;
    std::vector<Offset> offsets;
    auto inputWires = gate->GetParent();
    auto outs = gate->GetOutputWires();
    assert(inputWires.size() % outs.size() == 0);  // number of inputs is a multiple of number of output wires -> number of parents

    // gather all input wires
    for (size_t val = 0; val < inputWires.size(); ++val) {
        auto in1 = inputWires.at(val)->GetWireId();
        auto node1 = motionWireToFuseNode[in1];
        auto offset1 = motionWireToFuseOffset[in1];
        inputs.push_back(node1);
        offsets.push_back(offset1);
    }

    auto n = cb.addNodeWithCustomOperation("Simdify", inputs, offsets, outs.size() /*number of outputs*/);

    // map all output wires to the different offsets of the simdify node
    for (int i = 0; i < outs.size(); ++i) {
        auto out = outs.at(i)->GetWireId();
        motionWireToFuseNode[out] = n;
        motionWireToFuseOffset[out] = i;
    }
}

void MOTIONFrontendAdapter::translateUnsimdifyGate(frontend::CircuitBuilder& cb, UnsimdifyGatePointer gate) {
    // input: one wire to be split into single output wires without simd values
    // output: multiple output wires without simd values
    // different parents with the exact same amount of simd values per parent wire
    std::vector<Identifier> inputs;
    std::vector<Offset> offsets;
    auto inputWires = gate->GetParent();
    auto outs = gate->GetOutputWires();

    // gather all input wires
    for (size_t val = 0; val < inputWires.size(); ++val) {
        auto in1 = inputWires.at(val)->GetWireId();
        auto node1 = motionWireToFuseNode[in1];
        auto offset1 = motionWireToFuseOffset[in1];
        inputs.push_back(node1);
        offsets.push_back(offset1);
    }
    auto n = cb.addNodeWithCustomOperation("Unsimdify", inputs, offsets, outs.size() /*number of outputs*/);
    // map all output wires to the different offsets of the unsimdify node
    for (int i = 0; i < outs.size(); ++i) {
        auto out = outs.at(i)->GetWireId();
        motionWireToFuseNode[out] = n;
        motionWireToFuseOffset[out] = i;
    }
}

void MOTIONFrontendAdapter::translateSubsetGate(frontend::CircuitBuilder& cb, SubsetGatePointer gate) {
    std::vector<Identifier> inputs;
    std::vector<Offset> offsets;
    auto inputWires = gate->GetParent();
    auto outs = gate->GetOutputWires();

    // gather all input wires
    for (auto inputWire : inputWires) {
        auto in1 = inputWire->GetWireId();
        auto node1 = motionWireToFuseNode[in1];
        auto offset1 = motionWireToFuseOffset[in1];
        inputs.push_back(node1);
        offsets.push_back(offset1);
    }

    auto n = cb.addNodeWithCustomOperation("Subset", inputs, offsets, outs.size() /*number of outputs*/);

    // map all output wires to the different offsets of the unsimdify node
    for (int i = 0; i < outs.size(); ++i) {
        auto out = outs.at(i)->GetWireId();
        motionWireToFuseNode[out] = n;
        motionWireToFuseOffset[out] = i;
    }
}

void MOTIONFrontendAdapter::translateArithmeticGMWGate(frontend::CircuitBuilder& cb, encrypto::motion::GatePointer gate) {
    // Addition gates
    if (auto addGate = dynamic_pointer_cast<mo::proto::arithmetic_gmw::AdditionGate<uint8_t>>(gate)) {
        translateBinaryOperation(cb, addGate, fuse::core::ir::PrimitiveOperation::Add);
    } else if (auto addGate = dynamic_pointer_cast<mo::proto::arithmetic_gmw::AdditionGate<uint16_t>>(gate)) {
        translateBinaryOperation(cb, addGate, fuse::core::ir::PrimitiveOperation::Add);
    } else if (auto addGate = dynamic_pointer_cast<mo::proto::arithmetic_gmw::AdditionGate<uint32_t>>(gate)) {
        translateBinaryOperation(cb, addGate, fuse::core::ir::PrimitiveOperation::Add);
    } else if (auto addGate = dynamic_pointer_cast<mo::proto::arithmetic_gmw::AdditionGate<uint64_t>>(gate)) {
        translateBinaryOperation(cb, addGate, fuse::core::ir::PrimitiveOperation::Add);
    }
    // Subtraction gates
    else if (auto subGate = dynamic_pointer_cast<mo::proto::arithmetic_gmw::SubtractionGate<uint8_t>>(gate)) {
        translateBinaryOperation(cb, addGate, fuse::core::ir::PrimitiveOperation::Sub);
    } else if (auto subGate = dynamic_pointer_cast<mo::proto::arithmetic_gmw::SubtractionGate<uint16_t>>(gate)) {
        translateBinaryOperation(cb, addGate, fuse::core::ir::PrimitiveOperation::Sub);
    } else if (auto subGate = dynamic_pointer_cast<mo::proto::arithmetic_gmw::SubtractionGate<uint32_t>>(gate)) {
        translateBinaryOperation(cb, addGate, fuse::core::ir::PrimitiveOperation::Sub);
    } else if (auto subGate = dynamic_pointer_cast<mo::proto::arithmetic_gmw::SubtractionGate<uint64_t>>(gate)) {
        translateBinaryOperation(cb, addGate, fuse::core::ir::PrimitiveOperation::Sub);
    }
    // Multiplication gates
    else if (auto mulGate = dynamic_pointer_cast<mo::proto::arithmetic_gmw::MultiplicationGate<uint8_t>>(gate)) {
        translateBinaryOperation(cb, addGate, fuse::core::ir::PrimitiveOperation::Mul);
    } else if (auto mulGate = dynamic_pointer_cast<mo::proto::arithmetic_gmw::MultiplicationGate<uint16_t>>(gate)) {
        translateBinaryOperation(cb, addGate, fuse::core::ir::PrimitiveOperation::Mul);
    } else if (auto mulGate = dynamic_pointer_cast<mo::proto::arithmetic_gmw::MultiplicationGate<uint32_t>>(gate)) {
        translateBinaryOperation(cb, addGate, fuse::core::ir::PrimitiveOperation::Mul);
    } else if (auto mulGate = dynamic_pointer_cast<mo::proto::arithmetic_gmw::MultiplicationGate<uint64_t>>(gate)) {
        translateBinaryOperation(cb, addGate, fuse::core::ir::PrimitiveOperation::Mul);
    }
    // Hybrid Multiplication gates
    else if (auto hybridMulGate = dynamic_pointer_cast<mo::proto::arithmetic_gmw::HybridMultiplicationGate<uint8_t>>(gate)) {
        translateBinaryOperation(cb, addGate, fuse::core::ir::PrimitiveOperation::Mul);
    } else if (auto hybridMulGate = dynamic_pointer_cast<mo::proto::arithmetic_gmw::HybridMultiplicationGate<uint16_t>>(gate)) {
        translateBinaryOperation(cb, addGate, fuse::core::ir::PrimitiveOperation::Mul);
    } else if (auto hybridMulGate = dynamic_pointer_cast<mo::proto::arithmetic_gmw::HybridMultiplicationGate<uint32_t>>(gate)) {
        translateBinaryOperation(cb, addGate, fuse::core::ir::PrimitiveOperation::Mul);
    } else if (auto hybridMulGate = dynamic_pointer_cast<mo::proto::arithmetic_gmw::HybridMultiplicationGate<uint64_t>>(gate)) {
        translateBinaryOperation(cb, addGate, fuse::core::ir::PrimitiveOperation::Mul);
    }
    // Square gates: OneGate
    else if (auto squareGate = dynamic_pointer_cast<mo::proto::arithmetic_gmw::SquareGate<uint8_t>>(gate)) {
        translateUnaryOperation(cb, squareGate, fuse::core::ir::PrimitiveOperation::Square);
    } else if (auto squareGate = dynamic_pointer_cast<mo::proto::arithmetic_gmw::SquareGate<uint16_t>>(gate)) {
        translateUnaryOperation(cb, squareGate, fuse::core::ir::PrimitiveOperation::Square);
    } else if (auto squareGate = dynamic_pointer_cast<mo::proto::arithmetic_gmw::SquareGate<uint32_t>>(gate)) {
        translateUnaryOperation(cb, squareGate, fuse::core::ir::PrimitiveOperation::Square);
    } else if (auto squareGate = dynamic_pointer_cast<mo::proto::arithmetic_gmw::SquareGate<uint64_t>>(gate)) {
        translateUnaryOperation(cb, squareGate, fuse::core::ir::PrimitiveOperation::Square);
    } else {
        std::runtime_error("Unsupported gate operation for Arithmetic GMW protocol");
    }
}

void MOTIONFrontendAdapter::translateBMRGate(frontend::CircuitBuilder& cb, encrypto::motion::GatePointer gate) {
    fuse::core::ir::PrimitiveOperation op;
    if (auto g = dynamic_pointer_cast<mo::proto::bmr::XorGate>(gate)) {
        translateBinaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::Xor);
    } else if (auto g = dynamic_pointer_cast<mo::proto::bmr::InvGate>(gate)) {
        translateUnaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::Not);
    } else if (auto g = dynamic_pointer_cast<mo::proto::bmr::AndGate>(gate)) {
        translateBinaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::And);
    } else {
        std::runtime_error("Unsupported gate operation for BMR protocol");
    }
}

void MOTIONFrontendAdapter::translateAstraGate(frontend::CircuitBuilder& cb, encrypto::motion::GatePointer gate) {
    // Addition
    if (auto g = dynamic_pointer_cast<mo::proto::astra::AdditionGate<uint8_t>>(gate)) {
        translateBinaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::Add);
    } else if (auto g = dynamic_pointer_cast<mo::proto::astra::AdditionGate<uint16_t>>(gate)) {
        translateBinaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::Add);
    } else if (auto g = dynamic_pointer_cast<mo::proto::astra::AdditionGate<uint32_t>>(gate)) {
        translateBinaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::Add);
    } else if (auto g = dynamic_pointer_cast<mo::proto::astra::AdditionGate<uint64_t>>(gate)) {
        translateBinaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::Add);
    }

    // Subtraction
    else if (auto g = dynamic_pointer_cast<mo::proto::astra::SubtractionGate<uint8_t>>(gate)) {
        translateBinaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::Sub);
    } else if (auto g = dynamic_pointer_cast<mo::proto::astra::SubtractionGate<uint16_t>>(gate)) {
        translateBinaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::Sub);
    } else if (auto g = dynamic_pointer_cast<mo::proto::astra::SubtractionGate<uint32_t>>(gate)) {
        translateBinaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::Sub);
    } else if (auto g = dynamic_pointer_cast<mo::proto::astra::SubtractionGate<uint64_t>>(gate)) {
        translateBinaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::Sub);
    }

    // Multiplication
    else if (auto g = dynamic_pointer_cast<mo::proto::astra::MultiplicationGate<uint8_t>>(gate)) {
        translateBinaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::Mul);
    } else if (auto g = dynamic_pointer_cast<mo::proto::astra::MultiplicationGate<uint16_t>>(gate)) {
        translateBinaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::Mul);
    } else if (auto g = dynamic_pointer_cast<mo::proto::astra::MultiplicationGate<uint32_t>>(gate)) {
        translateBinaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::Mul);
    } else if (auto g = dynamic_pointer_cast<mo::proto::astra::MultiplicationGate<uint64_t>>(gate)) {
        translateBinaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::Mul);
    }

    else {
        std::runtime_error("Unsupported gate operation for Astra protocol");
    }
}

void MOTIONFrontendAdapter::translateBooleanGMWGate(frontend::CircuitBuilder& cb, encrypto::motion::GatePointer gate) {
    if (auto g = dynamic_pointer_cast<mo::proto::boolean_gmw::XorGate>(gate)) {
        translateBinaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::Xor);
    } else if (auto g = dynamic_pointer_cast<mo::proto::boolean_gmw::InvGate>(gate)) {
        translateUnaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::Not);
    } else if (auto g = dynamic_pointer_cast<mo::proto::boolean_gmw::AndGate>(gate)) {
        translateBinaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::And);
    } else if (auto g = dynamic_pointer_cast<mo::proto::boolean_gmw::MuxGate>(gate)) {
        translateTernaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::Mux);
    } else {
        std::runtime_error("Unsupported gate operation for Boolean GMW");
    }
}

void MOTIONFrontendAdapter::translateConstantBooleanGate(frontend::CircuitBuilder& cb, encrypto::motion::GatePointer gate) {
    // Constant Boolean Input
    if (auto constGate = dynamic_pointer_cast<mo::proto::ConstantBooleanInputGate>(gate)) {
        auto ws = constGate->GetOutputWires();
        std::vector<std::vector<bool>> constantMat;
        for (auto& w : ws) {
            std::vector<bool> simdConstant;
            auto constWire = dynamic_pointer_cast<mo::proto::ConstantBooleanWire>(w);
            assert(constWire);
            auto vals = constWire->GetValues();
            for (int bit = 0; bit < vals.GetSize(); ++bit) {
                simdConstant.push_back(vals.Get(bit));
            }
            constantMat.push_back(simdConstant);
        }

        auto nodeID = cb.addConstantNodeWithPayload(constantMat, "simd:" + std::to_string(ws.at(0)->GetNumberOfSimdValues()));

        Offset currentOffset = 0;
        for (auto& w : ws) {
            motionWireToFuseNode[w->GetWireId()] = nodeID;
            motionWireToFuseOffset[w->GetWireId()] = currentOffset++;
        }
    } else {
        std::runtime_error("Unsupported gate for translation of Boolean Constant");
    }
}

void MOTIONFrontendAdapter::translateConstantArithmeticGate(frontend::CircuitBuilder& cb, encrypto::motion::GatePointer gate) {
    // Constant Arithmetic Input
    if (auto constGate = dynamic_pointer_cast<mo::proto::ConstantArithmeticInputGate<uint8_t>>(gate)) {
        auto& wire = constGate->GetOutputWires().at(0);
        auto constWire = dynamic_pointer_cast<mo::proto::ConstantArithmeticWire<uint8_t>>(wire);
        std::vector<uint8_t> simdValues;
        auto vals = constWire->GetValues();
        for (int i = 0; i < wire->GetNumberOfSimdValues(); ++i) {
            simdValues.push_back(vals.at(i));
        }
        auto nodeID = cb.addConstantNodeWithPayload(simdValues, "simd:" + std::to_string(simdValues.size()));
        motionWireToFuseNode[wire->GetWireId()] = nodeID;
    } else if (auto g = dynamic_pointer_cast<mo::proto::ConstantArithmeticInputGate<uint16_t>>(gate)) {
        auto& wire = constGate->GetOutputWires().at(0);
        auto constWire = dynamic_pointer_cast<mo::proto::ConstantArithmeticWire<uint16_t>>(wire);
        std::vector<uint16_t> simdValues;
        auto vals = constWire->GetValues();
        for (int i = 0; i < wire->GetNumberOfSimdValues(); ++i) {
            simdValues.push_back(vals.at(i));
        }
        auto nodeID = cb.addConstantNodeWithPayload(simdValues, "simd:" + std::to_string(simdValues.size()));
        motionWireToFuseNode[wire->GetWireId()] = nodeID;
    } else if (auto g = dynamic_pointer_cast<mo::proto::ConstantArithmeticInputGate<uint32_t>>(gate)) {
        auto& wire = constGate->GetOutputWires().at(0);
        auto constWire = dynamic_pointer_cast<mo::proto::ConstantArithmeticWire<uint32_t>>(wire);
        std::vector<uint32_t> simdValues;
        auto vals = constWire->GetValues();
        for (int i = 0; i < wire->GetNumberOfSimdValues(); ++i) {
            simdValues.push_back(vals.at(i));
        }
        auto nodeID = cb.addConstantNodeWithPayload(simdValues, "simd:" + std::to_string(simdValues.size()));
        motionWireToFuseNode[wire->GetWireId()] = nodeID;
    } else if (auto g = dynamic_pointer_cast<mo::proto::ConstantArithmeticInputGate<uint64_t>>(gate)) {
        auto& wire = constGate->GetOutputWires().at(0);
        auto constWire = dynamic_pointer_cast<mo::proto::ConstantArithmeticWire<uint64_t>>(wire);
        std::vector<uint64_t> simdValues;
        auto vals = constWire->GetValues();
        for (int i = 0; i < wire->GetNumberOfSimdValues(); ++i) {
            simdValues.push_back(vals.at(i));
        }
        auto nodeID = cb.addConstantNodeWithPayload(simdValues, "simd:" + std::to_string(simdValues.size()));
        motionWireToFuseNode[wire->GetWireId()] = nodeID;
    }

    // Addition with Constant
    else if (auto g = dynamic_pointer_cast<mo::proto::ConstantArithmeticAdditionGate<uint8_t>>(gate)) {
        translateBinaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::Add, "const");
    } else if (auto g = dynamic_pointer_cast<mo::proto::ConstantArithmeticAdditionGate<uint16_t>>(gate)) {
        translateBinaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::Add, "const");
    } else if (auto g = dynamic_pointer_cast<mo::proto::ConstantArithmeticAdditionGate<uint32_t>>(gate)) {
        translateBinaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::Add, "const");
    } else if (auto g = dynamic_pointer_cast<mo::proto::ConstantArithmeticAdditionGate<uint64_t>>(gate)) {
        translateBinaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::Add, "const");
    }

    // Multiplication with Constant
    else if (auto g = dynamic_pointer_cast<mo::proto::ConstantArithmeticMultiplicationGate<uint8_t>>(gate)) {
        translateBinaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::Mul, "const");
    } else if (auto g = dynamic_pointer_cast<mo::proto::ConstantArithmeticMultiplicationGate<uint16_t>>(gate)) {
        translateBinaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::Mul, "const");
    } else if (auto g = dynamic_pointer_cast<mo::proto::ConstantArithmeticMultiplicationGate<uint32_t>>(gate)) {
        translateBinaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::Mul, "const");
    } else if (auto g = dynamic_pointer_cast<mo::proto::ConstantArithmeticMultiplicationGate<uint64_t>>(gate)) {
        translateBinaryOperation(cb, g, fuse::core::ir::PrimitiveOperation::Mul, "const");
    } else {
        std::runtime_error("Unsupported gate for translation of Arithmetic Constant");
    }
}

void MOTIONFrontendAdapter::translateConversionGate(frontend::CircuitBuilder& cb, encrypto::motion::GatePointer gate) {
    // Boolean GMW to Arithmetic GMW
    // -> merge input boolean wires to a single arithmetic value using Merge node
    bool b2a = false;
    if (auto g = dynamic_pointer_cast<mo::GmwToArithmeticGate<uint8_t>>(gate)) {
        bool b2a = true;
        assert(g->GetParent().size() == 8);  // 8 boolean input wires
    } else if (auto g = dynamic_pointer_cast<mo::GmwToArithmeticGate<uint16_t>>(gate)) {
        bool b2a = true;
        assert(g->GetParent().size() == 16);  // 16 boolean input wires
    } else if (auto g = dynamic_pointer_cast<mo::GmwToArithmeticGate<uint32_t>>(gate)) {
        bool b2a = true;
        assert(g->GetParent().size() == 32);  // 32 boolean input wires
    } else if (auto g = dynamic_pointer_cast<mo::GmwToArithmeticGate<uint64_t>>(gate)) {
        bool b2a = true;
        assert(g->GetParent().size() == 64);  // 64 boolean input wires
    }
    if (b2a) {
        auto g = dynamic_pointer_cast<mo::OneGate>(gate);
        assert(g);
        auto ins = g->GetParent();
        auto outs = g->GetOutputWires();
        assert(outs.size() == 1);  // one single arithmetic output wire
        std::vector<Identifier> inputs;
        std::vector<Offset> offsets;
        for (auto& bit : ins) {
            inputs.push_back(motionWireToFuseNode[bit->GetWireId()]);
            offsets.push_back(motionWireToFuseNode[bit->GetWireId()]);
        }
        auto mergeNode = cb.addNode(core::ir::PrimitiveOperation::Merge, inputs, offsets);
        motionWireToFuseNode[outs.at(0)->GetWireId()] = mergeNode;
    }

    // BMR (Y) to Boolean GMW
    // -> no node needed, store generated output wire for the same node output
    else if (auto g = dynamic_pointer_cast<mo::BmrToBooleanGmwGate>(gate)) {
        assert(g->GetParent().size() == g->GetOutputWires().size());
        for (std::size_t wire = 0; wire < g->GetOutputWires().size(); ++wire) {
            motionWireToFuseNode[g->GetOutputWires().at(wire)->GetWireId()] = motionWireToFuseNode[g->GetParent().at(wire)->GetWireId()];
            motionWireToFuseOffset[g->GetOutputWires().at(wire)->GetWireId()] = motionWireToFuseOffset[g->GetParent().at(wire)->GetWireId()];
        }
    }

    // Boolean GMW to BMR (Y)
    // -> no node needed, store generated output wire for the same node output
    else if (auto g = dynamic_pointer_cast<mo::BooleanGmwToBmrGate>(gate)) {
        assert(g->GetParent().size() == g->GetOutputWires().size());
        for (std::size_t wire = 0; wire < g->GetOutputWires().size(); ++wire) {
            motionWireToFuseNode[g->GetOutputWires().at(wire)->GetWireId()] = motionWireToFuseNode[g->GetParent().at(wire)->GetWireId()];
            motionWireToFuseOffset[g->GetOutputWires().at(wire)->GetWireId()] = motionWireToFuseOffset[g->GetParent().at(wire)->GetWireId()];
        }
    }

    // Arithmetic GMW to BMR
    // -> use Split node to split arithmetic input into boolean wires
    else if (auto g = dynamic_pointer_cast<mo::ArithmeticGmwToBmrGate>(gate)) {
        assert(g->GetParent().size() == 1);  // arithmetic input --> only one wire
        auto input = motionWireToFuseNode.at(g->GetParent().at(0)->GetWireId());
        auto bitlen = g->GetParent().at(0)->GetBitLength();
        auto type = bitlenToType(bitlen);
        auto splitNode = cb.addSplitNode(type, input);
        auto outWires = g->GetOutputWires();
        for (size_t bit = 0; bit < bitlen; ++bit) {
            auto w = outWires.at(bit);
            motionWireToFuseNode[w->GetWireId()] = splitNode;
            motionWireToFuseOffset[w->GetWireId()] = bit;
        }
    } else {
        std::runtime_error("Unsupported gate operation called for in conversion translation");
    }
}

void MOTIONFrontendAdapter::translateOutputGate(frontend::CircuitBuilder& cb, encrypto::motion::OutputGatePointer gate) {
    auto inWires = gate->GetParent();
    std::vector<Identifier> inputs;
    std::vector<Offset> offsets;
    std::vector<size_t> input_types;

    // output only contains a single input value
    for (auto& in : inWires) {
        if (!motionWireToFuseNode.contains(in->GetWireId())) {
            return;
        }
        inputs.push_back(motionWireToFuseNode[in->GetWireId()]);
        offsets.push_back(motionWireToFuseOffset[in->GetWireId()]);
    }

    if (inWires.at(0)->GetCircuitType() == mo::CircuitType::kBoolean) {
        switch (inWires.size()) {
            case 1:
                cb.addOutputNode(ptBool, inputs, offsets);
                break;
            case 8:
                cb.addOutputNode(ptBool8, inputs, offsets);
                break;
            case 16:
                cb.addOutputNode(ptBool16, inputs, offsets);
                break;
            case 32:
                cb.addOutputNode(ptBool32, inputs, offsets);
                break;
            case 64:
                cb.addOutputNode(ptBool64, inputs, offsets);
                break;
            default:
                throw std::runtime_error("Unsupported number of wires for boolean output: " + std::to_string(inWires.size()));
        }
    } else {  // arithmetic
        assert(inWires.size() == 1);
        switch (inWires.at(0)->GetBitLength()) {
            case 8:
                cb.addOutputNode(ptUint8, inputs, offsets);
                break;
            case 16:
                cb.addOutputNode(ptUint16, inputs, offsets);
                break;
            case 32:
                cb.addOutputNode(ptUint32, inputs, offsets);
                break;
            case 64:
                cb.addOutputNode(ptUint64, inputs, offsets);
                break;
            default:
                throw std::runtime_error("Unsupported number of bitlength for arithmetic output: " + std::to_string(inWires.at(0)->GetBitLength()));
        }
    }
}

void MOTIONFrontendAdapter::translateInputGate(frontend::CircuitBuilder& cb, encrypto::motion::InputGatePointer gate) {
    // input node has no input wires
    auto outs = gate->GetOutputWires();
    auto numWires = outs.size();
    auto numSimd = outs.at(0)->GetNumberOfSimdValues();
    auto circType = outs.at(0)->GetCircuitType();
    auto bitlen = outs.at(0)->GetBitLength();

    Identifier nodeID;

    std::string simdAnnotation = "simd:" + std::to_string(numSimd);
    if (circType == mo::CircuitType::kArithmetic) {
        assert(numWires == 1);
        switch (bitlen) {
            case 8:
                nodeID = cb.addInputNode(secUint8, simdAnnotation);
                break;
            case 16:
                nodeID = cb.addInputNode(secUint16, simdAnnotation);
                break;
            case 32:
                nodeID = cb.addInputNode(secUint32, simdAnnotation);
                break;
            case 64:
                nodeID = cb.addInputNode(secUint64, simdAnnotation);
                break;
            default:
                throw std::runtime_error("illegal bitlength for arithmetic input: " + std::to_string(bitlen));
        }
    } else if (circType == mo::CircuitType::kBoolean) {
        switch (numWires) {
            case 1:
                nodeID = cb.addInputNode(secBool, simdAnnotation);
                break;
            case 8:
                nodeID = cb.addInputNode(secBool8, simdAnnotation);
                break;
            case 16:
                nodeID = cb.addInputNode(secBool16, simdAnnotation);
                break;
            case 32:
                nodeID = cb.addInputNode(secBool32, simdAnnotation);
                break;
            case 64:
                nodeID = cb.addInputNode(secBool64, simdAnnotation);
                break;
            default:
                throw std::runtime_error("illegal number of wires for boolean input: " + std::to_string(numWires));
        }
    } else {
        throw std::runtime_error("Invalid circuit type for input gate");
    }

    for (int i = 0; i < outs.size(); ++i) {
        motionWireToFuseNode[outs.at(i)->GetWireId()] = nodeID;
        motionWireToFuseOffset[outs.at(i)->GetWireId()] = i;
    }
}

void MOTIONFrontendAdapter::translateMOTIONGate(frontend::CircuitBuilder& cb, encrypto::motion::GatePointer gate) {
    if (auto inGate = dynamic_pointer_cast<mo::InputGate>(gate)) {
        translateInputGate(cb, inGate);
        return;
    } else if (auto outGate = dynamic_pointer_cast<mo::OutputGate>(gate)) {
        translateOutputGate(cb, outGate);
        return;
    }
    // Data management gates
    if (auto g = dynamic_pointer_cast<mo::SimdifyGate>(gate)) {
        translateSimdifyGate(cb, g);
        return;
    } else if (auto g = dynamic_pointer_cast<mo::SubsetGate>(gate)) {
        translateSubsetGate(cb, g);
        return;
    } else if (auto g = dynamic_pointer_cast<mo::UnsimdifyGate>(gate)) {
        translateUnsimdifyGate(cb, g);
        return;
    }

    auto protocol = gate->GetOutputWires()[0]->GetProtocol();
    switch (protocol) {
        case mo::MpcProtocol::kArithmeticGmw: {
            translateArithmeticGMWGate(cb, gate);
            return;
        }
        case mo::MpcProtocol::kArithmeticConstant: {
            translateConstantArithmeticGate(cb, gate);
            return;
        }
        case mo::MpcProtocol::kBmr: {
            translateBMRGate(cb, gate);
            return;
        }
        case mo::MpcProtocol::kBooleanConstant: {
            translateConstantBooleanGate(cb, gate);
            return;
        }
        case mo::MpcProtocol::kBooleanGmw: {
            translateBooleanGMWGate(cb, gate);
            return;
        }
        case mo::MpcProtocol::kAstra: {
            translateAstraGate(cb, gate);
        }
        default:
            throw std::runtime_error("No translation available, unknown protocol");
    }
    // Conversion gates left
    translateConversionGate(cb, gate);
}

void MOTIONFrontendAdapter::initDataTypes(frontend::CircuitBuilder& cb) {
    // single bits
    secBool = cb.addDataType(fuse::core::ir::PrimitiveType::Bool);
    ptBool = cb.addDataType(fuse::core::ir::PrimitiveType::Bool, fuse::core::ir::SecurityLevel::Plaintext);
    // bitvectors
    secBool8 = cb.addDataType(fuse::core::ir::PrimitiveType::Bool, fuse::core::ir::SecurityLevel::Secure, {8});
    ptBool8 = cb.addDataType(fuse::core::ir::PrimitiveType::Bool, fuse::core::ir::SecurityLevel::Plaintext, {8});
    ptBool16 = cb.addDataType(fuse::core::ir::PrimitiveType::Bool, fuse::core::ir::SecurityLevel::Plaintext, {16});
    secBool16 = cb.addDataType(fuse::core::ir::PrimitiveType::Bool, fuse::core::ir::SecurityLevel::Secure, {16});
    ptBool32 = cb.addDataType(fuse::core::ir::PrimitiveType::Bool, fuse::core::ir::SecurityLevel::Plaintext, {32});
    secBool32 = cb.addDataType(fuse::core::ir::PrimitiveType::Bool, fuse::core::ir::SecurityLevel::Secure, {32});
    ptBool64 = cb.addDataType(fuse::core::ir::PrimitiveType::Bool, fuse::core::ir::SecurityLevel::Plaintext, {64});
    secBool64 = cb.addDataType(fuse::core::ir::PrimitiveType::Bool, fuse::core::ir::SecurityLevel::Secure, {64});
    // arithmetic input with given bitlength
    secUint8 = cb.addDataType(fuse::core::ir::PrimitiveType::UInt8);
    ptUint8 = cb.addDataType(fuse::core::ir::PrimitiveType::UInt8, fuse::core::ir::SecurityLevel::Plaintext);
    secUint16 = cb.addDataType(fuse::core::ir::PrimitiveType::UInt16);
    ptUint16 = cb.addDataType(fuse::core::ir::PrimitiveType::UInt16, fuse::core::ir::SecurityLevel::Plaintext);
    secUint32 = cb.addDataType(fuse::core::ir::PrimitiveType::UInt32);
    ptUint32 = cb.addDataType(fuse::core::ir::PrimitiveType::UInt32, fuse::core::ir::SecurityLevel::Plaintext);
    ptUint64 = cb.addDataType(fuse::core::ir::PrimitiveType::UInt64);
    ptUint64 = cb.addDataType(fuse::core::ir::PrimitiveType::UInt64, fuse::core::ir::SecurityLevel::Plaintext);
}

core::CircuitContext loadFUSEFromMOTION(encrypto::motion::PartyPointer& party, const std::string& circuitName) {
    frontend::CircuitBuilder cb(circuitName);
    // add all relevant data types in advance

    auto& motionRegister = party->GetBackend()->GetRegister();
    MOTIONFrontendAdapter frontend;
    frontend.initDataTypes(cb);

    for (auto& gate : motionRegister->GetGates()) {
        frontend.translateMOTIONGate(cb, gate);
    }

    return core::CircuitContext(cb);
}

}  // namespace fuse::frontend
