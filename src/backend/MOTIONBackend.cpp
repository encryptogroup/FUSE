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

// FUSE
#include "MOTIONBackend.h"

#include <regex>

// MOTION
#include "base/backend.h"
#include "base/party.h"
#include "protocols/arithmetic_gmw/arithmetic_gmw_share.h"
#include "protocols/astra/astra_share.h"
#include "protocols/bmr/bmr_share.h"
#include "protocols/boolean_gmw/boolean_gmw_share.h"
#include "protocols/constant/constant_gate.h"
#include "protocols/constant/constant_share.h"
#include "protocols/share_wrapper.h"

namespace fuse::backend {

    ShareVector
    MOTIONBackend::getInputSharesForNode(const core::NodeReadOnly &node) const {
        ShareVector inputShares;
        auto inputNodes = node.getInputNodeIDs();

        for (size_t inputIndex = 0; inputIndex < node.getNumberOfInputs();
             ++inputIndex) {
            auto inputNodeID = inputNodes[inputIndex];
            if (nodesToMotionShare.contains(inputNodeID)) {
                auto shareVariant = nodesToMotionShare.at(inputNodeID);
                // node holds single share
                if (std::holds_alternative<Share>(shareVariant)) {
                    inputShares.push_back(std::get<Share>(shareVariant));
                }
                // node holds vector of shares: select the right one or copy complete
                // vector
                else if (std::holds_alternative<ShareVector>(shareVariant)) {
                    auto shareVariantAsVector = std::get<ShareVector>(shareVariant);
                    if (node.usesInputOffsets()) {
                        inputShares.push_back(
                            shareVariantAsVector.at(node.getInputOffsets()[inputIndex]));
                    } else {
                        inputShares.insert(inputShares.end(), shareVariantAsVector.begin(),
                                           shareVariantAsVector.end());
                    }
                }
                // node holds invalid value: throw exception
                else {
                    assert(shareVariant.valueless_by_exception());
                    throw motion_error("Input Share is valueless for node with ID: " +
                                       std::to_string(inputNodeID));
                }
            } else {
                throw motion_error(
                    "Could not find MOTION input share for node with ID: " +
                    std::to_string(inputNodeID));
            }
        }
        return inputShares;
    }

    void MOTIONBackend::visit(const core::NodeReadOnly &node) {
        if (nodesToMotionShare.contains(node.getNodeID())) {
            // this node's share has already been computed, no need to compute it again
            return;
        }

        // get input shares from the input nodes
        auto inputShares = getInputSharesForNode(node);
        std::variant<Share, ShareVector> nodeOutput;

        // create gate with corresponding gate operation using the operation on the
        // ShareWrapper
        using op = core::ir::PrimitiveOperation;
        switch (node.getOperation()) {
        // Operations supported by MOTION natively
        case op::Not: {
            assert(inputShares.size() == 1);
            nodeOutput = ~inputShares.at(0);
            break;
        }
        case op::Xor: {
            assert(inputShares.size() == 2);
            nodeOutput = inputShares.at(0) ^ inputShares.at(1);
            break;
        }
        case op::And: {
            assert(inputShares.size() == 2);
            nodeOutput = inputShares.at(0) & inputShares.at(1);
            break;
        }
        case op::Or: {
            assert(inputShares.size() == 2);
            nodeOutput = inputShares.at(0) | inputShares.at(1);
            break;
        }
        case op::Add: {
            assert(inputShares.size() == 2);
            nodeOutput = inputShares.at(0) + inputShares.at(1);
            break;
        }
        case op::Sub: {
            assert(inputShares.size() == 2);
            nodeOutput = inputShares.at(0) - inputShares.at(1);
            break;
        }
        case op::Mul: {
            assert(inputShares.size() == 2);
            nodeOutput = inputShares.at(0) * inputShares.at(1);
            break;
        }
        case op::Square: {
            // TODO
        }
        case op::Eq: {
            assert(inputShares.size() == 2);
            nodeOutput = inputShares.at(0) == inputShares.at(1);
            break;
        }
        case op::Mux: {
            assert(inputShares.size() == 3);
            nodeOutput = inputShares.at(0).Mux(inputShares.at(1), inputShares.at(2));
            break;
        }
        case op::Split: {
            assert(inputShares.size() == 1);
            nodeOutput = inputShares.at(0).Split();
            break;
        }
        case op::Merge: {
            nodeOutput = encrypto::motion::ShareWrapper::Concatenate(inputShares);
            break;
        }

        // input, output, constants
        case op::Input:
            throw motion_error("Missing input share for input node with ID: " +
                               node.getNodeID());
        case op::Output: {
            assert(inputShares.size() == 1);
            nodeOutput = inputShares.at(0);
            break;
        }
        case op::Constant: {
            throw motion_error("not yet implemented");
        }

        // Operations not supported by MOTION natively: translate to supported ones
        case op::Nand: {
            assert(inputShares.size() == 2);
            nodeOutput = ~(inputShares.at(0) & inputShares.at(1));
            break;
        }
        case op::Nor: {
            assert(inputShares.size() == 2);
            nodeOutput = ~(inputShares.at(0) | inputShares.at(1));
            break;
        }
        case op::Xnor: {
            assert(inputShares.size() == 2);
            nodeOutput = ~(inputShares.at(0) ^ inputShares.at(1));
            break;
        }
        case op::CallSubcircuit: {
            // TODO if necessary with hycc
        }

        case op::Custom: {
            // TODO
            // Simdify
            // Unsimdify
            // Subset
        }

        case op::Loop:
            throw motion_error("not yet implemented");

        // Operations not supportable: throw exception
        case op::Div:
        case op::Gt:
        case op::Ge:
        case op::Lt:
        case op::Le:
        default: {
            throw motion_error("Unsupported operation for MOTION at node with ID: " +
                               std::to_string(node.getNodeID()));
        }
        }

        // put wrapper in nodesToMotionShare
        nodesToMotionShare[node.getNodeID()] = nodeOutput;
    }

    ShareVector MOTIONBackend::evaluate(const core::CircuitReadOnly &circuit) {
        circuit.topologicalTraversal(
            [=, this](const core::NodeReadOnly &node) { visit(node); });
        ShareVector result;
        result.reserve(circuit.getNumberOfOutputs());
        for (auto output : circuit.getOutputNodeIDs()) {
            auto outputShareVariant = nodesToMotionShare.at(output);
            // if output holds single share : just append to result vector
            if (std::holds_alternative<Share>(outputShareVariant)) {
                result.push_back(std::get<Share>(outputShareVariant));
            }
            // if output holds vector of shares : add all shares to result
            else if (std::holds_alternative<ShareVector>(outputShareVariant)) {
                auto vectorAlternative = std::get<ShareVector>(outputShareVariant);
                result.insert(result.end(), vectorAlternative.begin(),
                              vectorAlternative.end());
            }
            // in this case, there was no value found for the output
            else {
                assert(outputShareVariant.valueless_by_exception());
                throw motion_error("Output Share is valueless for node with ID: " +
                                   std::to_string(output));
            }
        }
        return result;
    }

    void MOTIONBackend::visit(const core::ModuleReadOnly &module) {
        // handle global inputs and outputs
        throw motion_error("not yet implemented");
    }

    /*
    *************************************************************************************************************************
    *************************************** Another try at a MOTION Backend
    **************************************************
    *************************************************************************************************************************
    */

    namespace mo = encrypto::motion;
    using Offset = uint32_t;

    struct Environment
    {
        std::unordered_map<Identifier, ShareVector> nodeToOutputShares{};
    };

    /**
     * @brief Construct a Share wrapper instance from the given wires
     *
     * @param wires vector of wires from which a wrapped share will be constructed
     * @return ShareWrapper that contains the actual wires for computation
     */
    Share wiresToShareWrapper(const std::vector<mo::WirePointer> &wires) {
        auto w = wires.at(0);

        switch (w->GetProtocol()) {
        case mo::MpcProtocol::kArithmeticGmw: {
            assert(wires.size() == 1);
            switch (w->GetBitLength()) {
            case 8: {
                return mo::ShareWrapper(
                    std::make_shared<mo::proto::arithmetic_gmw::Share<uint8_t>>(w));
            }
            case 16: {
                return mo::ShareWrapper(
                    std::make_shared<mo::proto::arithmetic_gmw::Share<uint16_t>>(w));
            }
            case 32: {
                return mo::ShareWrapper(
                    std::make_shared<mo::proto::arithmetic_gmw::Share<uint32_t>>(w));
            }
            case 64: {
                return mo::ShareWrapper(
                    std::make_shared<mo::proto::arithmetic_gmw::Share<uint64_t>>(w));
            }
            }
        }
        case mo::MpcProtocol::kAstra: {
            assert(wires.size() == 1);
            switch (w->GetBitLength()) {
            case 8: {
                return mo::ShareWrapper(
                    std::make_shared<mo::proto::astra::Share<uint8_t>>(w));
            }
            case 16: {
                return mo::ShareWrapper(
                    std::make_shared<mo::proto::astra::Share<uint16_t>>(w));
            }
            case 32: {
                return mo::ShareWrapper(
                    std::make_shared<mo::proto::astra::Share<uint32_t>>(w));
            }
            case 64: {
                return mo::ShareWrapper(
                    std::make_shared<mo::proto::astra::Share<uint64_t>>(w));
            }
            }
        }
        case mo::MpcProtocol::kBmr: {
            return mo::ShareWrapper(std::make_shared<mo::proto::bmr::Share>(wires));
        }
        case mo::MpcProtocol::kBooleanGmw: {
            return mo::ShareWrapper(
                std::make_shared<mo::proto::boolean_gmw::Share>(wires));
        }
        default:
            throw std::runtime_error("Unsupported protocol to create shares from wires "
                                     "in FUSE MOTION Backend");
        }
    }

    /**
     * @brief Initialize the environment for input nodes given the input gates from
     * the MOTION party.
     *
     * @param circuit contains the input nodes
     * @param party contains the input gates
     * @param env stores the mapping from nodes to shares
     */
    void evaluateInputGates(const core::CircuitReadOnly &circuit,
                            const mo::PartyPointer &party, Environment &env) {
        auto inputGates = party->GetBackend()->GetRegister()->GetGates();
        auto inputNodes = circuit.getInputNodeIDs();
        assert(inputNodes.size() == inputGates.size());

        for (int i = 0; i < inputNodes.size(); ++i) {
            // get current input gate and input node
            auto &inGate = inputGates.at(i);
            auto inNodeID = inputNodes[i];
            auto inNode = circuit.getNodeWithID(inNodeID);
            // set input node the the results of the corresponding MOTION input gate
            auto outs = inGate->GetOutputWires();
            auto w = outs.at(0);
            switch (w->GetProtocol()) {
            case mo::MpcProtocol::kAstra: {
                // only one output wire
                assert(outs.size() == 1);
                switch (w->GetBitLength()) {
                case 8: {
                    env.nodeToOutputShares[inNodeID].push_back(mo::ShareWrapper(
                        std::make_shared<mo::proto::astra::Share<uint8_t>>(w)));
                    break;
                }
                case 16: {
                    env.nodeToOutputShares[inNodeID].push_back(mo::ShareWrapper(
                        std::make_shared<mo::proto::astra::Share<uint16_t>>(w)));
                    break;
                }
                case 32: {
                    env.nodeToOutputShares[inNodeID].push_back(mo::ShareWrapper(
                        std::make_shared<mo::proto::astra::Share<uint32_t>>(w)));
                    break;
                }
                case 64: {
                    env.nodeToOutputShares[inNodeID].push_back(mo::ShareWrapper(
                        std::make_shared<mo::proto::astra::Share<uint64_t>>(w)));
                    break;
                }
                default: {
                    throw std::runtime_error("Invalid bitlength for Astra input gate");
                }
                }
                break;
            }
            case mo::MpcProtocol::kArithmeticGmw: {
                // only one output wire
                assert(outs.size() == 1);
                switch (w->GetBitLength()) {
                case 8: {
                    env.nodeToOutputShares[inNodeID].push_back(mo::ShareWrapper(
                        std::make_shared<mo::proto::arithmetic_gmw::Share<uint8_t>>(w)));
                    break;
                }
                case 16: {
                    env.nodeToOutputShares[inNodeID].push_back(mo::ShareWrapper(
                        std::make_shared<mo::proto::arithmetic_gmw::Share<uint16_t>>(w)));
                    break;
                }
                case 32: {
                    env.nodeToOutputShares[inNodeID].push_back(mo::ShareWrapper(
                        std::make_shared<mo::proto::arithmetic_gmw::Share<uint32_t>>(w)));
                    break;
                }
                case 64: {
                    env.nodeToOutputShares[inNodeID].push_back(mo::ShareWrapper(
                        std::make_shared<mo::proto::arithmetic_gmw::Share<uint64_t>>(w)));
                    break;
                }
                default: {
                    throw std::runtime_error("Invalid bitlength for A-GMW input gate");
                }
                }
                break;
            }
            case mo::MpcProtocol::kBooleanGmw: {
                // construct a single boolean share that holds all the wires
                auto boolShare = mo::ShareWrapper(
                    std::make_shared<mo::proto::boolean_gmw::Share>(outs));
                env.nodeToOutputShares[inNodeID].push_back(boolShare);
                break;
            }
            case mo::MpcProtocol::kBmr: {
                // construct a single boolean share that holds all the wires
                auto boolShare =
                    mo::ShareWrapper(std::make_shared<mo::proto::bmr::Share>(outs));
                env.nodeToOutputShares[inNodeID].push_back(boolShare);
                break;
            }
            default:
                throw std::runtime_error("Invalid protocol for input gate");
            }
        }
    } // namespace fuse::backend

    void evaluateNode(const core::CircuitReadOnly &parentCircuit,
                      const core::NodeReadOnly &node, const mo::PartyPointer &party,
                      Environment &env);

    void checkIfValuesPresent(const core::CircuitReadOnly &parentCircuit,
                              uint64_t nodeID, const mo::PartyPointer &party,
                              Environment &env) {
        if (!env.nodeToOutputShares.contains(nodeID)) {
            evaluateNode(parentCircuit, *parentCircuit.getNodeWithID(nodeID), party,
                         env);
        }
    }

    std::array<Share, 3>
    getSimdifiedMuxInputs(const core::CircuitReadOnly &parentCircuit,
                          const core::NodeReadOnly &node,
                          const mo::PartyPointer &party, Environment &env) {
        ShareVector simd1;
        ShareVector simd2;
        ShareVector simd3;
        // inputs correspond to the wires -> gather all wires for one share, then
        // construct share from this
        auto inputs = node.getInputNodeIDs();
        auto offsets = node.getInputOffsets();

        // condition values
        auto condSize = std::stoi(node.getStringValueForAttribute("cond"));
        auto valSize = std::stoi(node.getStringValueForAttribute("val"));
        int singleMuxValue = condSize + (2 * valSize);
        assert(node.getNumberOfInputs() == singleMuxValue);

        // [ cond shares    |  val1 shares  | val2 shares ]
        //     condSize         valSize         valSize
        int counter = -1;
        for (int i = 0; i < node.getNumberOfInputs(); ++i) {
            ++counter;
            if (counter == singleMuxValue) {
                counter = 0;
            }
            auto in = inputs[i];
            auto offset = offsets.empty() ? 0 : offsets[i];

            checkIfValuesPresent(parentCircuit, in, party, env);

            if (counter < condSize) {
                simd1.push_back(env.nodeToOutputShares[in].at(offset));
            } else if (counter < condSize + valSize) {
                simd2.push_back(env.nodeToOutputShares[in].at(offset));
            } else {
                simd3.push_back(env.nodeToOutputShares[in].at(offset));
            }
        }
        return {mo::ShareWrapper::Simdify(simd1), mo::ShareWrapper::Simdify(simd2),
                mo::ShareWrapper::Simdify(simd3)};
    }

    std::array<Share, 2>
    getBinarySimdifiedInputs(const core::CircuitReadOnly &parentCircuit,
                             const core::NodeReadOnly &node,
                             const mo::PartyPointer &party, Environment &env) {
        ShareVector simd1;
        ShareVector simd2;
        // inputs correspond to the wires -> gather all wires for one share, then
        // construct share from this
        auto inputs = node.getInputNodeIDs();
        auto offsets = node.getInputOffsets();

        for (int i = 0; i < node.getNumberOfInputs(); ++i) {
            auto in = inputs[i];
            auto offset = offsets.empty() ? 0 : offsets[i];

            checkIfValuesPresent(parentCircuit, in, party, env);

            if (i % 2 == 0) {
                simd1.push_back(env.nodeToOutputShares.at(in).at(offset));
            } else {
                simd2.push_back(env.nodeToOutputShares.at(in).at(offset));
            }
        }
        return {mo::ShareWrapper::Simdify(simd1), mo::ShareWrapper::Simdify(simd2)};
    }

    Share getUnarySimdifiedInputs(const core::CircuitReadOnly &parentCircuit,
                                  const core::NodeReadOnly &node,
                                  const mo::PartyPointer &party, Environment &env) {
        ShareVector simd;
        // inputs correspond to the wires -> gather all wires for one share, then
        // construct share from this
        auto inputs = node.getInputNodeIDs();
        auto offsets = node.getInputOffsets();

        for (int i = 0; i < node.getNumberOfInputs(); ++i) {
            auto in = inputs[i];
            auto offset = offsets.empty() ? 0 : offsets[i];

            checkIfValuesPresent(parentCircuit, in, party, env);

            simd.push_back(env.nodeToOutputShares.at(in).at(offset));
        }
        return mo::ShareWrapper::Simdify(simd);
    }

    Share translateConstant(const core::NodeReadOnly &node,
                            const mo::PartyPointer &party, Environment &env) {
        using t = fuse::core::ir::PrimitiveType;
        auto backend = party->GetBackend();
        switch (node.getConstantType().get()->getPrimitiveType()) {
        case t::Bool: {
            return mo::proto::ConstantBooleanInputGate(node.getConstantBool(), *backend)
                .GetOutputAsShare();
        }
        case t::Int8: // MOTION only supports unsigned values
        case t::UInt8: {
            return mo::proto::ConstantArithmeticInputGate<uint8_t>(
                       {node.getConstantUInt8()}, *backend)
                .GetOutputAsShare();
        }
        case t::Int16: // MOTION only supports unsigned values
        case t::UInt16: {
            return mo::proto::ConstantArithmeticInputGate<uint16_t>(
                       {node.getConstantUInt16()}, *backend)
                .GetOutputAsShare();
        }
        case t::Int32: // MOTION only supports unsigned values
        case t::UInt32: {
            return mo::proto::ConstantArithmeticInputGate<uint32_t>(
                       {node.getConstantUInt32()}, *backend)
                .GetOutputAsShare();
        }
        case t::Int64: // MOTION only supports unsigned values
        case t::UInt64: {
            return mo::proto::ConstantArithmeticInputGate<uint64_t>(
                       {node.getConstantUInt64()}, *backend)
                .GetOutputAsShare();
        }
        default: {
            throw std::runtime_error(
                "MOTION does not support floating points and double constants!");
        }
        }
    }

    ShareVector getInputShares(const core::CircuitReadOnly &parentCircuit,
                               const core::NodeReadOnly &node, Environment &env,
                               const mo::PartyPointer &party,
                               int numberOfInputShares) {
        ShareVector res;
        int numberOfWiresPerInputShare =
            node.getNumberOfInputs() / numberOfInputShares;
        res.reserve(numberOfInputShares);

        // inputs correspond to the wires -> gather all wires for one share, then
        // construct share from this
        auto inputs = node.getInputNodeIDs();
        auto offsets = node.getInputOffsets();

        assert(inputs.size() == numberOfInputShares * numberOfWiresPerInputShare);

        /*
            |   |       |   | (in11 in12 (first share)  in21 in22 (second share))
                [AND]
                |   |
        */
        for (int i = 0; i < numberOfInputShares; ++i) {
            std::vector<mo::WirePointer> wiresForShare;
            for (int w = 0; w < numberOfWiresPerInputShare; ++w) {
                auto inNode = inputs[i * w];
                auto inOffset = offsets.empty() ? 0 : offsets[i * w];

                checkIfValuesPresent(parentCircuit, inNode, party, env);

                // add wires for share to be constructed
                auto inShareWires =
                    env.nodeToOutputShares.at(inNode).at(inOffset).Get()->GetWires();
                wiresForShare.insert(wiresForShare.end(), inShareWires.begin(),
                                     inShareWires.end());
            }
            Share currentInShareToConstruct = wiresToShareWrapper(wiresForShare);
            res.push_back(currentInShareToConstruct);
        }

        return res;
    } // namespace fuse::backend

    std::array<Share, 3> getMuxInputs(const core::CircuitReadOnly &parentCircuit,
                                      const core::NodeReadOnly &node,
                                      const mo::PartyPointer &party,
                                      Environment &env) {
        throw std::runtime_error("Not implemented yet!");
    }

    void evaluateNode(const core::CircuitReadOnly &parentCircuit,
                      const core::NodeReadOnly &node, const mo::PartyPointer &party,
                      Environment &env) {
        if (env.nodeToOutputShares.contains(
                node.getNodeID())) { // node output already computed -> nothing to do
            return;
        }

        Identifier nodeId = node.getNodeID();
        ShareVector nodeOutput;

        using op = core::ir::PrimitiveOperation;
        switch (node.getOperation()) {
        case op::Not: {
            if (node.getNumberOfInputs() == 1) { // normal operation
                auto inputs = getInputShares(parentCircuit, node, env, party, 1);
                nodeOutput.push_back(~inputs[0]);
            } else { // SIMD node
                assert(node.getNumberOfInputs() == node.getNumberOfOutputs());
                auto input = getUnarySimdifiedInputs(parentCircuit, node, party, env);
                auto invOutput = ~input;
                nodeOutput = invOutput.Unsimdify();
            }
            break;
        }
        case op::Xor: {
            if (node.getNumberOfInputs() == 2) {
                auto inputs = getInputShares(parentCircuit, node, env, party, 2);
                nodeOutput.push_back(inputs[0] ^ inputs[1]);
            } else { // SIMD node
                assert(node.getNumberOfInputs() == 2 * node.getNumberOfOutputs());
                auto inputs = getBinarySimdifiedInputs(parentCircuit, node, party, env);
                auto opOut = inputs[0] ^ inputs[1];
                nodeOutput = opOut.Unsimdify();
            }
            break;
        }
        case op::And: {
            if (node.getNumberOfInputs() == 2) {
                auto inputs = getInputShares(parentCircuit, node, env, party, 2);
                nodeOutput.push_back(inputs[0] & inputs[1]);
            } else { // SIMD Node
                assert(node.getNumberOfInputs() == 2 * node.getNumberOfOutputs());
                auto inputs = getBinarySimdifiedInputs(parentCircuit, node, party, env);
                auto andOutSimd = inputs[0] & inputs[1];
                nodeOutput = andOutSimd.Unsimdify();
            }
            break;
        }
        case op::Or: {
            if (node.getNumberOfInputs() == 2) {
                auto inputs = getInputShares(parentCircuit, node, env, party, 2);
                nodeOutput.push_back(inputs[0] | inputs[1]);
            } else { // SIMD node
                assert(node.getNumberOfInputs() == 2 * node.getNumberOfOutputs());
                auto inputs = getBinarySimdifiedInputs(parentCircuit, node, party, env);
                auto opOut = inputs[0] | inputs[1];
                nodeOutput = opOut.Unsimdify();
            }
            break;
        }
        case op::Add: {
            if (node.getNumberOfInputs() == 2) {
                auto inputs = getInputShares(parentCircuit, node, env, party, 2);
                nodeOutput.push_back(inputs[0] + inputs[1]);
            } else { // SIMD node
                assert(node.getNumberOfInputs() == 2 * node.getNumberOfOutputs());
                auto inputs = getBinarySimdifiedInputs(parentCircuit, node, party, env);
                auto opOut = inputs[0] + inputs[1];
                nodeOutput = opOut.Unsimdify();
            }
            break;
        }
        case op::Sub: {
            if (node.getNumberOfInputs() == 2) {
                auto inputs = getInputShares(parentCircuit, node, env, party, 2);
                nodeOutput.push_back(inputs[0] - inputs[1]);
            } else { // SIMD node
                assert(node.getNumberOfInputs() == 2 * node.getNumberOfOutputs());
                auto inputs = getBinarySimdifiedInputs(parentCircuit, node, party, env);
                auto opOut = inputs[0] - inputs[1];
                nodeOutput = opOut.Unsimdify();
            }
            break;
        }
        case op::Mul: {
            if (node.getNumberOfInputs() == 2) {
                auto inputs = getInputShares(parentCircuit, node, env, party, 2);
                nodeOutput.push_back(inputs[0] * inputs[1]);
            } else { // SIMD node
                assert(node.getNumberOfInputs() == 2 * node.getNumberOfOutputs());
                auto inputs = getBinarySimdifiedInputs(parentCircuit, node, party, env);
                auto opOut = inputs[0] * inputs[1];
                nodeOutput = opOut.Unsimdify();
            }
            break;
        }
        case op::Square: {
            if (node.getNumberOfInputs() == 1) { // normal operation
                auto inputs = getInputShares(parentCircuit, node, env, party, 1);
                nodeOutput.push_back(inputs[0] * inputs[0]);
            } else { // SIMD node
                assert(node.getNumberOfInputs() == node.getNumberOfOutputs());
                auto input = getUnarySimdifiedInputs(parentCircuit, node, party, env);
                auto opOut = input * input;
                nodeOutput = opOut.Unsimdify();
            }
            break;
        }
        case op::Eq: {
            if (node.getNumberOfInputs() == 2) {
                auto inputs = getInputShares(parentCircuit, node, env, party, 2);
                nodeOutput.push_back(inputs[0] == inputs[1]);
            } else { // SIMD node
                assert(node.getNumberOfInputs() == 2 * node.getNumberOfOutputs());
                auto inputs = getBinarySimdifiedInputs(parentCircuit, node, party, env);
                auto opOut = inputs[0] == inputs[1];
                nodeOutput = opOut.Unsimdify();
            }
            break;
        }
        case op::Mux: {
            // this ? a : b
            if (node.getNumberOfInputs() == 3) {
                auto inputs = getInputShares(parentCircuit, node, env, party, 3);
                nodeOutput.push_back(inputs[0].Mux(inputs[1], inputs[2]));
            } else { // SIMD node
                assert(node.getNumberOfInputs() == 2 * node.getNumberOfOutputs());
                auto inputs = getSimdifiedMuxInputs(parentCircuit, node, party, env);
                auto opOut = inputs[0].Mux(inputs[1], inputs[2]);
                nodeOutput = opOut.Unsimdify();
            }
            break;
        }
        case op::Split: {
            auto inputs = getInputShares(parentCircuit, node, env, party, 1);
            nodeOutput = inputs[0].Split();
            break;
        }
        case op::Merge: {
            auto inputs = getInputShares(parentCircuit, node, env, party,
                                         node.getNumberOfInputs());
            nodeOutput.push_back(mo::ShareWrapper::Concatenate(inputs));
            break;
        }
        case op::Custom: {
            auto opName = node.getCustomOperationName();
            if (opName == "Simdify") {
                auto inputs = getInputShares(parentCircuit, node, env, party,
                                             node.getNumberOfInputs());
                nodeOutput.push_back(mo::ShareWrapper::Simdify(inputs));
            } else if (opName == "Unsimdify") {
                auto inputs = getInputShares(parentCircuit, node, env, party, 1);
                nodeOutput = inputs.at(0).Unsimdify();
            } else {
                throw std::runtime_error(
                    "Unsupported CUSTOM operation for MOTION backend at node with ID: " +
                    std::to_string(node.getNodeID()) + " called " + opName);
            }
            break;
        }
        case op::Output: {
            auto inputs = getInputShares(parentCircuit, node, env, party, 1);
            nodeOutput.push_back(inputs[0].Out());
            break;
        }
        case op::Constant: {
            nodeOutput.push_back(translateConstant(node, party, env));
            break;
        }
        case op::CallSubcircuit: {
            throw std::runtime_error("Tried to resolve call for circuit without a "
                                     "module in MOTION backend for node with ID: " +
                                     std::to_string(node.getNodeID()));
        }
        default: {
            throw std::runtime_error(
                "Unsupported operation for MOTION backend at node with ID: " +
                std::to_string(node.getNodeID()));
        }
        }
        env.nodeToOutputShares[nodeId] = nodeOutput;
    }

    std::unordered_map<Identifier, ShareVector>
    evaluate(const core::CircuitReadOnly &circuit, const mo::PartyPointer &party) {
        Environment env;
        evaluateInputGates(circuit, party, env);
        circuit.topologicalTraversal([&](const core::NodeReadOnly &node) {
    if (!node.isInputNode()) {
      evaluateNode(circuit, node, party, env);
    } });
        return env.nodeToOutputShares;
    }

    void evaluateCircuit(const core::CircuitReadOnly &circuit,
                         const core::ModuleReadOnly &parentModule,
                         const mo::PartyPointer &party, Environment &env);

    void evaluateCall(const core::NodeReadOnly &node,
                      const core::ModuleReadOnly &parentModule,
                      const mo::PartyPointer &party, Environment &env) {
        if (env.nodeToOutputShares.contains(
                node.getNodeID())) { // node output already computed -> nothing to do
            return;
        }
        Identifier nodeId = node.getNodeID();
        auto callee = parentModule.getCircuitWithName(node.getSubCircuitName());

        // prepare input environment
        Environment calleeEnv;
        auto inputShares = env.nodeToOutputShares[nodeId];
        auto numOfInputs = callee->getNumberOfInputs();
        assert(inputShares.size() == numOfInputs);
        for (int i = 0; i < numOfInputs; ++i) {
            calleeEnv.nodeToOutputShares[callee->getInputNodeIDs()[i]].push_back(
                inputShares.at(i));
        }

        // evaluate subcircuit
        evaluateCircuit(*callee, parentModule, party, calleeEnv);

        // read out output values from call
        ShareVector nodeOutput;
        for (auto calleeOutputID : callee->getOutputNodeIDs()) {
            auto outputVec = calleeEnv.nodeToOutputShares[calleeOutputID];
            assert(outputVec.size() == 1);
            nodeOutput.push_back(outputVec[0]);
        }

        env.nodeToOutputShares[nodeId] = nodeOutput;
    }

    void evaluateCircuit(const core::CircuitReadOnly &circuit,
                         const core::ModuleReadOnly &parentModule,
                         const mo::PartyPointer &party, Environment &env) {
        circuit.topologicalTraversal([&](const core::NodeReadOnly &node) {
    if (!node.isInputNode()) {
      if (node.isSubcircuitNode()) {
        evaluateCall(node, parentModule, party, env);
      } else {
        evaluateNode(circuit, node, party, env);
      }
    } });
    }

    std::unordered_map<Identifier, ShareVector>
    evaluate(const core::ModuleReadOnly &module, const mo::PartyPointer &party,
             const core::CircuitReadOnly &main) {
        Environment env;
        evaluateInputGates(main, party, env);
        evaluateCircuit(main, module, party, env);
        return env.nodeToOutputShares;
    }

} // namespace fuse::backend
