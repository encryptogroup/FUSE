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

#include "ConstantFolder.h"

#include <functional>
#include <numeric>
#include <unordered_set>

#include "PrimitiveOperationPolicies.hpp"
#include "PrimitiveTypeTraits.hpp"

namespace fuse::passes {
class ConstantFolder {
    using Identifier = uint64_t;

   public:
    void visit(core::CircuitObjectWrapper& circuit);

   private:
    std::unordered_map<Identifier, flexbuffers::Reference> constantNodes_;
};

/*
*****************************************************************************************************************************
*/

template <core::ir::PrimitiveType T,
          typename AccumulationPolicy,
          typename Traits = core::PrimitiveTypeTraits<T>>
void computeAndStoreAccumulation(core::NodeObjectWrapper& node, std::span<const flexbuffers::Reference> inputs) {
    using AccumulationType = typename Traits::AccumulationType;
    assert(inputs.size() > 0);
    // compute accumulated value
    AccumulationType accumulator = inputs[0].As<AccumulationType>();
    for (auto it = inputs.begin() + 1, end = inputs.end(); it != end; ++it) {
        AccumulationPolicy::accumulate(accumulator, it->As<AccumulationType>());
    }
    // change node to constant node and save constant
    node.setInputNodeIDs({});
    node.setPrimitiveOperation(core::ir::PrimitiveOperation::Constant);
    // static cast to highest value s.t. the correct setPayload function can be called
    node.setPayload(static_cast<typename Traits::PayloadType>(accumulator));
    node.setConstantType(T);
}

template <core::ir::PrimitiveType T,
          typename AccumulationPolicy,
          typename Traits = core::PrimitiveTypeTraits<T>>
void computeAndStoreInvertedAccumulation(core::NodeObjectWrapper& node, std::span<const flexbuffers::Reference> inputs) {
    using AccumulationType = typename Traits::AccumulationType;
    assert(inputs.size() > 0);
    // compute accumulated value
    AccumulationType accumulator = inputs[0].As<AccumulationType>();
    for (auto it = inputs.begin() + 1, end = inputs.end(); it != end; ++it) {
        AccumulationPolicy::accumulate(accumulator, it->As<AccumulationType>());
    }
    // invert accumulated value
    core::PrimitiveOperationPolicies<core::ir::PrimitiveOperation::Not>::accumulate(accumulator);
    // change node to constant node and save constant
    node.setInputNodeIDs({});
    node.setPrimitiveOperation(core::ir::PrimitiveOperation::Constant);
    // static cast to highest value s.t. the correct setPayload function can be called
    node.setPayload(static_cast<typename Traits::PayloadType>(accumulator));
    node.setConstantType(T);
}

template <core::ir::PrimitiveType T,
          typename ComparatorPolicy,
          typename Traits = core::PrimitiveTypeTraits<T>>
void computeAndStoreComparisonOperation(core::NodeObjectWrapper& node, std::span<const flexbuffers::Reference> inputs) {
    using AccumulationType = typename Traits::AccumulationType;
    assert(inputs.size() == 2);
    // compute comparison value
    auto comparisonVal = ComparatorPolicy::template apply<AccumulationType>(inputs[0].As<AccumulationType>(), inputs[1].As<AccumulationType>());
    // change node to constant node and save constant
    node.setInputNodeIDs({});
    node.setPrimitiveOperation(core::ir::PrimitiveOperation::Constant);
    // static cast to highest value s.t. the correct setPayload function can be called
    node.setPayload(static_cast<typename Traits::PayloadType>(comparisonVal));
    node.setConstantType(T);
}

template <core::ir::PrimitiveType T,
          typename OperationPolicy,
          typename Traits = core::PrimitiveTypeTraits<T>>
void computeAndStoreUnaryOperation(core::NodeObjectWrapper& node, std::span<const flexbuffers::Reference> inputs) {
    using AccumulationType = typename Traits::AccumulationType;
    assert(inputs.size() == 1);
    // compute comparison value
    auto comparisonVal = OperationPolicy::template apply<AccumulationType>(inputs[0].As<AccumulationType>());
    // change node to constant node and save constant
    node.setInputNodeIDs({});
    node.setPrimitiveOperation(core::ir::PrimitiveOperation::Constant);
    // static cast to highest value s.t. the correct setPayload function can be called
    node.setPayload(static_cast<typename Traits::PayloadType>(comparisonVal));
    node.setConstantType(T);
}

template <core::ir::PrimitiveType T,
          typename OperationPolicy,
          core::ir::PrimitiveType Cond = core::ir::PrimitiveType::Bool,
          typename Traits = core::PrimitiveTypeTraits<T>>
void computeAndStoreMuxOperation(core::NodeObjectWrapper& node, std::span<const flexbuffers::Reference> inputs) {
    using AccumulationType = typename Traits::AccumulationType;
    using CondType = typename core::PrimitiveTypeTraits<Cond>::AccumulationType;
    assert(inputs.size() == 3);
    // compute comparison value
    auto comparisonVal = OperationPolicy::template apply<AccumulationType>(inputs[0].As<CondType>(), inputs[1].As<AccumulationType>(), inputs[2].As<AccumulationType>());
    // change node to constant node and save constant
    node.setInputNodeIDs({});
    node.setPrimitiveOperation(core::ir::PrimitiveOperation::Constant);
    // static cast to highest value s.t. the correct setPayload function can be called
    node.setPayload(static_cast<typename Traits::PayloadType>(comparisonVal));
    node.setConstantType(T);
}

template <core::ir::PrimitiveType T,
          typename OperationPolicy,
          typename Traits = core::PrimitiveTypeTraits<T>>
void computeAndStoreSplitOperation(core::NodeObjectWrapper& node, std::span<const flexbuffers::Reference> inputs) {
    using AccumulationType = typename Traits::AccumulationType;
    assert(inputs.size() == 1);
    // compute comparison value : in this special case, the output type will be std::bitset<>
    auto bitSet = OperationPolicy::template apply<AccumulationType, Traits::NumBits>(inputs[0].As<AccumulationType>());
    std::vector<bool> res;
    for (size_t it = 0, end = bitSet.size(); it != end; ++it) {
        res.push_back(bitSet[0]);
    }
    // change node to constant node and save constant
    node.setInputNodeIDs({});
    node.setPrimitiveOperation(core::ir::PrimitiveOperation::Constant);
    node.setPayload(res);
    long shapeDim = res.size();
    node.setConstantType(T, std::array<int64_t, 1>{shapeDim});
}

template <core::ir::PrimitiveType T,
          typename OperationPolicy,
          typename Traits = core::PrimitiveTypeTraits<T>>
void computeAndStoreMergeOperation(core::NodeObjectWrapper& node, std::span<const flexbuffers::Reference> inputs) {
    using AccumulationType = typename Traits::AccumulationType;
    assert(inputs.size() > 1);
    std::vector<bool> inputBits;
    size_t numberOfInputs = inputs.size();
    for (auto input : inputs) {
        inputBits.push_back(input.As<bool>());
    }

    auto resVal = OperationPolicy::template apply<AccumulationType>(inputBits);

    // change node to constant node and save constant
    node.setInputNodeIDs({});
    node.setPrimitiveOperation(core::ir::PrimitiveOperation::Constant);
    // static cast to highest value s.t. the correct setPayload function can be called
    node.setPayload(static_cast<typename Traits::PayloadType>(resVal));
    node.setConstantType(T);
}

/*
*****************************************************************************************************************************
*/

template <typename AccumulationPolicy>
void visitBooleanAccumulation(core::ir::PrimitiveType primitiveType, core::NodeObjectWrapper& node, std::span<const flexbuffers::Reference> inputs) {
    using pt = core::ir::PrimitiveType;
    switch (primitiveType) {
        case pt::Bool:
            computeAndStoreAccumulation<pt::Bool, AccumulationPolicy>(node, inputs);
            return;
        case pt::Int8:
            computeAndStoreAccumulation<pt::Int8, AccumulationPolicy>(node, inputs);
            return;
        case pt::Int16:
            computeAndStoreAccumulation<pt::Int16, AccumulationPolicy>(node, inputs);
            return;
        case pt::Int32:
            computeAndStoreAccumulation<pt::Int32, AccumulationPolicy>(node, inputs);
            return;
        case pt::Int64:
            computeAndStoreAccumulation<pt::Int64, AccumulationPolicy>(node, inputs);
            return;
        case pt::UInt8:
            computeAndStoreAccumulation<pt::UInt8, AccumulationPolicy>(node, inputs);
            return;
        case pt::UInt16:
            computeAndStoreAccumulation<pt::UInt16, AccumulationPolicy>(node, inputs);
            return;
        case pt::UInt32:
            computeAndStoreAccumulation<pt::UInt32, AccumulationPolicy>(node, inputs);
            return;
        case pt::UInt64:
            computeAndStoreAccumulation<pt::UInt64, AccumulationPolicy>(node, inputs);
            return;
        case pt::Float:
        case pt::Double:
        default:
            std::string dtName = core::ir::EnumNamePrimitiveType(primitiveType);
            throw std::logic_error("unexpected datatype for operator: " + dtName);
    }
}

template <typename AccumulationPolicy>
void visitArithmeticAccumulation(core::ir::PrimitiveType primitiveType, core::NodeObjectWrapper& node, std::span<const flexbuffers::Reference> inputs) {
    using pt = core::ir::PrimitiveType;
    switch (primitiveType) {
        case pt::Bool:
            computeAndStoreAccumulation<pt::Bool, AccumulationPolicy>(node, inputs);
            return;
        case pt::Int8:
            computeAndStoreAccumulation<pt::Int8, AccumulationPolicy>(node, inputs);
            return;
        case pt::Int16:
            computeAndStoreAccumulation<pt::Int16, AccumulationPolicy>(node, inputs);
            return;
        case pt::Int32:
            computeAndStoreAccumulation<pt::Int32, AccumulationPolicy>(node, inputs);
            return;
        case pt::Int64:
            computeAndStoreAccumulation<pt::Int64, AccumulationPolicy>(node, inputs);
            return;
        case pt::UInt8:
            computeAndStoreAccumulation<pt::UInt8, AccumulationPolicy>(node, inputs);
            return;
        case pt::UInt16:
            computeAndStoreAccumulation<pt::UInt16, AccumulationPolicy>(node, inputs);
            return;
        case pt::UInt32:
            computeAndStoreAccumulation<pt::UInt32, AccumulationPolicy>(node, inputs);
            return;
        case pt::UInt64:
            computeAndStoreAccumulation<pt::UInt64, AccumulationPolicy>(node, inputs);
            return;
        case pt::Float:
            computeAndStoreAccumulation<pt::Float, AccumulationPolicy>(node, inputs);
            return;
        case pt::Double:
            computeAndStoreAccumulation<pt::Double, AccumulationPolicy>(node, inputs);
            return;
        default:
            std::string dtName = core::ir::EnumNamePrimitiveType(primitiveType);
            throw std::logic_error("unexpected datatype for operator: " + dtName);
    }
}

template <typename AccumulationPolicy>
void visitAccumulateAndInvertOperation(core::ir::PrimitiveType primitiveType, core::NodeObjectWrapper& node, std::span<const flexbuffers::Reference> inputs) {
    using pt = core::ir::PrimitiveType;
    switch (primitiveType) {
        case pt::Bool:
            computeAndStoreInvertedAccumulation<pt::Bool, AccumulationPolicy>(node, inputs);
            return;
        case pt::Int8:
            computeAndStoreInvertedAccumulation<pt::Int8, AccumulationPolicy>(node, inputs);
            return;
        case pt::Int16:
            computeAndStoreInvertedAccumulation<pt::Int16, AccumulationPolicy>(node, inputs);
            return;
        case pt::Int32:
            computeAndStoreInvertedAccumulation<pt::Int32, AccumulationPolicy>(node, inputs);
            return;
        case pt::Int64:
            computeAndStoreInvertedAccumulation<pt::Int64, AccumulationPolicy>(node, inputs);
            return;
        case pt::UInt8:
            computeAndStoreInvertedAccumulation<pt::UInt8, AccumulationPolicy>(node, inputs);
            return;
        case pt::UInt16:
            computeAndStoreInvertedAccumulation<pt::UInt16, AccumulationPolicy>(node, inputs);
            return;
        case pt::UInt32:
            computeAndStoreInvertedAccumulation<pt::UInt32, AccumulationPolicy>(node, inputs);
            return;
        case pt::UInt64:
            computeAndStoreInvertedAccumulation<pt::UInt64, AccumulationPolicy>(node, inputs);
            return;
        case pt::Float:
        case pt::Double:
        default:
            std::string dtName = core::ir::EnumNamePrimitiveType(primitiveType);
            throw std::logic_error("unexpected datatype for operation: " + dtName);
    }
}

template <typename ComparatorPolicy>
void visitComparisonOperation(core::ir::PrimitiveType primitiveType, core::NodeObjectWrapper& node, std::span<const flexbuffers::Reference> inputs) {
    using pt = core::ir::PrimitiveType;
    switch (primitiveType) {
        case pt::Bool:
            computeAndStoreComparisonOperation<pt::Bool, ComparatorPolicy>(node, inputs);
            return;
        case pt::Int8:
            computeAndStoreComparisonOperation<pt::Int8, ComparatorPolicy>(node, inputs);
            return;
        case pt::Int16:
            computeAndStoreComparisonOperation<pt::Int16, ComparatorPolicy>(node, inputs);
            return;
        case pt::Int32:
            computeAndStoreComparisonOperation<pt::Int32, ComparatorPolicy>(node, inputs);
            return;
        case pt::Int64:
            computeAndStoreComparisonOperation<pt::Int64, ComparatorPolicy>(node, inputs);
            return;
        case pt::UInt8:
            computeAndStoreComparisonOperation<pt::UInt8, ComparatorPolicy>(node, inputs);
            return;
        case pt::UInt16:
            computeAndStoreComparisonOperation<pt::UInt16, ComparatorPolicy>(node, inputs);
            return;
        case pt::UInt32:
            computeAndStoreComparisonOperation<pt::UInt32, ComparatorPolicy>(node, inputs);
            return;
        case pt::UInt64:
            computeAndStoreComparisonOperation<pt::UInt64, ComparatorPolicy>(node, inputs);
            return;
        case pt::Float:
            computeAndStoreComparisonOperation<pt::Float, ComparatorPolicy>(node, inputs);
            return;
        case pt::Double:
            computeAndStoreComparisonOperation<pt::Double, ComparatorPolicy>(node, inputs);
            return;
        default:
            std::string dtName = core::ir::EnumNamePrimitiveType(primitiveType);
            throw std::logic_error("unexpected datatype for comparison operator: " + dtName);
    }
}

void visitNegation(core::ir::PrimitiveType primitiveType, core::NodeObjectWrapper& node, std::span<const flexbuffers::Reference> inputs) {
    using pt = core::ir::PrimitiveType;
    using op = core::ir::PrimitiveOperation;
    switch (primitiveType) {
        case pt::Bool:
            computeAndStoreUnaryOperation<pt::Bool, core::PrimitiveOperationPolicies<op::Neg>>(node, inputs);
            return;
        case pt::Int8:
            computeAndStoreUnaryOperation<pt::Int8, core::PrimitiveOperationPolicies<op::Neg>>(node, inputs);
            return;
        case pt::Int16:
            computeAndStoreUnaryOperation<pt::Int16, core::PrimitiveOperationPolicies<op::Neg>>(node, inputs);
            return;
        case pt::Int32:
            computeAndStoreUnaryOperation<pt::Int32, core::PrimitiveOperationPolicies<op::Neg>>(node, inputs);
            return;
        case pt::Int64:
            computeAndStoreUnaryOperation<pt::Int64, core::PrimitiveOperationPolicies<op::Neg>>(node, inputs);
            return;
        case pt::UInt8:
            computeAndStoreUnaryOperation<pt::UInt8, core::PrimitiveOperationPolicies<op::Neg>>(node, inputs);
            return;
        case pt::UInt16:
            computeAndStoreUnaryOperation<pt::UInt16, core::PrimitiveOperationPolicies<op::Neg>>(node, inputs);
            return;
        case pt::UInt32:
            computeAndStoreUnaryOperation<pt::UInt32, core::PrimitiveOperationPolicies<op::Neg>>(node, inputs);
            return;
        case pt::UInt64:
            computeAndStoreUnaryOperation<pt::UInt64, core::PrimitiveOperationPolicies<op::Neg>>(node, inputs);
            return;
        case pt::Float:
            computeAndStoreUnaryOperation<pt::Float, core::PrimitiveOperationPolicies<op::Neg>>(node, inputs);
            return;
        case pt::Double:
            computeAndStoreUnaryOperation<pt::Double, core::PrimitiveOperationPolicies<op::Neg>>(node, inputs);
            return;
        default:
            std::string dtName = core::ir::EnumNamePrimitiveType(primitiveType);
            throw std::logic_error("unexpected datatype for operation: " + dtName);
    }
}

void visitSplit(core::ir::PrimitiveType primitiveType, core::NodeObjectWrapper& node, std::span<const flexbuffers::Reference> inputs) {
    using pt = core::ir::PrimitiveType;
    using op = core::ir::PrimitiveOperation;
    switch (primitiveType) {
        case pt::Bool:
            computeAndStoreSplitOperation<pt::Bool, core::PrimitiveOperationPolicies<op::Split>>(node, inputs);
            return;
        case pt::Int8:
            computeAndStoreSplitOperation<pt::Int8, core::PrimitiveOperationPolicies<op::Split>>(node, inputs);
            return;
        case pt::Int16:
            computeAndStoreSplitOperation<pt::Int16, core::PrimitiveOperationPolicies<op::Split>>(node, inputs);
            return;
        case pt::Int32:
            computeAndStoreSplitOperation<pt::Int32, core::PrimitiveOperationPolicies<op::Split>>(node, inputs);
            return;
        case pt::Int64:
            computeAndStoreSplitOperation<pt::Int64, core::PrimitiveOperationPolicies<op::Split>>(node, inputs);
            return;
        case pt::UInt8:
            computeAndStoreSplitOperation<pt::UInt8, core::PrimitiveOperationPolicies<op::Split>>(node, inputs);
            return;
        case pt::UInt16:
            computeAndStoreSplitOperation<pt::UInt16, core::PrimitiveOperationPolicies<op::Split>>(node, inputs);
            return;
        case pt::UInt32:
            computeAndStoreSplitOperation<pt::UInt32, core::PrimitiveOperationPolicies<op::Split>>(node, inputs);
            return;
        case pt::UInt64:
            computeAndStoreSplitOperation<pt::UInt64, core::PrimitiveOperationPolicies<op::Split>>(node, inputs);
            return;
        /*
        case pt::Float:
            computeAndStoreSplitOperation<pt::Float, core::PrimitiveOperationPolicies<op::Split>>(node, inputs);
            return;
        case pt::Double:
            computeAndStoreSplitOperation<pt::Double, core::PrimitiveOperationPolicies<op::Split>>(node, inputs);
            return;
        */
        default:
            std::string dtName = core::ir::EnumNamePrimitiveType(primitiveType);
            throw std::logic_error("unexpected datatype for operation: " + dtName);
    }
}

void visitMerge(core::ir::PrimitiveType primitiveType, core::NodeObjectWrapper& node, std::span<const flexbuffers::Reference> inputs) {
    using pt = core::ir::PrimitiveType;
    using op = core::ir::PrimitiveOperation;
    switch (primitiveType) {
        case pt::Bool:
            computeAndStoreMergeOperation<pt::Bool, core::PrimitiveOperationPolicies<op::Merge>>(node, inputs);
            return;
        case pt::Int8:
            computeAndStoreMergeOperation<pt::Int8, core::PrimitiveOperationPolicies<op::Merge>>(node, inputs);
            return;
        case pt::Int16:
            computeAndStoreMergeOperation<pt::Int16, core::PrimitiveOperationPolicies<op::Merge>>(node, inputs);
            return;
        case pt::Int32:
            computeAndStoreMergeOperation<pt::Int32, core::PrimitiveOperationPolicies<op::Merge>>(node, inputs);
            return;
        case pt::Int64:
            computeAndStoreMergeOperation<pt::Int64, core::PrimitiveOperationPolicies<op::Merge>>(node, inputs);
            return;
        case pt::UInt8:
            computeAndStoreMergeOperation<pt::UInt8, core::PrimitiveOperationPolicies<op::Merge>>(node, inputs);
            return;
        case pt::UInt16:
            computeAndStoreMergeOperation<pt::UInt16, core::PrimitiveOperationPolicies<op::Merge>>(node, inputs);
            return;
        case pt::UInt32:
            computeAndStoreMergeOperation<pt::UInt32, core::PrimitiveOperationPolicies<op::Merge>>(node, inputs);
            return;
        case pt::UInt64:
            computeAndStoreMergeOperation<pt::UInt64, core::PrimitiveOperationPolicies<op::Merge>>(node, inputs);
            return;
        /*
        case pt::Float:
            computeAndStoreMergeOperation<pt::Float, core::PrimitiveOperationPolicies<op::Merge>>(node, inputs);
            return;
        case pt::Double:
            computeAndStoreMergeOperation<pt::Double, core::PrimitiveOperationPolicies<op::Merge>>(node, inputs);
            return;
        */
        default:
            std::string dtName = core::ir::EnumNamePrimitiveType(primitiveType);
            throw std::logic_error("unexpected datatype for operation: " + dtName);
    }
}

void visitNot(core::ir::PrimitiveType primitiveType, core::NodeObjectWrapper& node, std::span<const flexbuffers::Reference> inputs) {
    using pt = core::ir::PrimitiveType;
    using op = core::ir::PrimitiveOperation;
    switch (primitiveType) {
        case pt::Bool:
            computeAndStoreUnaryOperation<pt::Bool, core::PrimitiveOperationPolicies<op::Not>>(node, inputs);
            return;
        case pt::Int8:
            computeAndStoreUnaryOperation<pt::Int8, core::PrimitiveOperationPolicies<op::Not>>(node, inputs);
            return;
        case pt::Int16:
            computeAndStoreUnaryOperation<pt::Int16, core::PrimitiveOperationPolicies<op::Not>>(node, inputs);
            return;
        case pt::Int32:
            computeAndStoreUnaryOperation<pt::Int32, core::PrimitiveOperationPolicies<op::Not>>(node, inputs);
            return;
        case pt::Int64:
            computeAndStoreUnaryOperation<pt::Int64, core::PrimitiveOperationPolicies<op::Not>>(node, inputs);
            return;
        case pt::UInt8:
            computeAndStoreUnaryOperation<pt::UInt8, core::PrimitiveOperationPolicies<op::Not>>(node, inputs);
            return;
        case pt::UInt16:
            computeAndStoreUnaryOperation<pt::UInt16, core::PrimitiveOperationPolicies<op::Not>>(node, inputs);
            return;
        case pt::UInt32:
            computeAndStoreUnaryOperation<pt::UInt32, core::PrimitiveOperationPolicies<op::Not>>(node, inputs);
            return;
        case pt::UInt64:
            computeAndStoreUnaryOperation<pt::UInt64, core::PrimitiveOperationPolicies<op::Not>>(node, inputs);
            return;
        case pt::Float:
        case pt::Double:
        default:
            std::string dtName = core::ir::EnumNamePrimitiveType(primitiveType);
            throw std::logic_error("unexpected datatype for operation: " + dtName);
    }
}

void visitMux(core::ir::PrimitiveType primitiveType, core::NodeObjectWrapper& node, std::span<const flexbuffers::Reference> inputs) {
    using pt = core::ir::PrimitiveType;
    using op = core::ir::PrimitiveOperation;
    switch (primitiveType) {
        case pt::Bool:
            computeAndStoreMuxOperation<pt::Bool, core::PrimitiveOperationPolicies<op::Mux>>(node, inputs);
            return;
        case pt::Int8:
            computeAndStoreMuxOperation<pt::Int8, core::PrimitiveOperationPolicies<op::Mux>>(node, inputs);
            return;
        case pt::Int16:
            computeAndStoreMuxOperation<pt::Int16, core::PrimitiveOperationPolicies<op::Mux>>(node, inputs);
            return;
        case pt::Int32:
            computeAndStoreMuxOperation<pt::Int32, core::PrimitiveOperationPolicies<op::Mux>>(node, inputs);
            return;
        case pt::Int64:
            computeAndStoreMuxOperation<pt::Int64, core::PrimitiveOperationPolicies<op::Mux>>(node, inputs);
            return;
        case pt::UInt8:
            computeAndStoreMuxOperation<pt::UInt8, core::PrimitiveOperationPolicies<op::Mux>>(node, inputs);
            return;
        case pt::UInt16:
            computeAndStoreMuxOperation<pt::UInt16, core::PrimitiveOperationPolicies<op::Mux>>(node, inputs);
            return;
        case pt::UInt32:
            computeAndStoreMuxOperation<pt::UInt32, core::PrimitiveOperationPolicies<op::Mux>>(node, inputs);
            return;
        case pt::UInt64:
            computeAndStoreMuxOperation<pt::UInt64, core::PrimitiveOperationPolicies<op::Mux>>(node, inputs);
            return;
        case pt::Float:
        case pt::Double:
        default:
            std::string dtName = core::ir::EnumNamePrimitiveType(primitiveType);
            throw std::logic_error("unexpected datatype for operation: " + dtName);
    }
}
/*
 **********************************************************************************************************************
 */

/**
 * @brief Folds the constants inside the circuit.
 *
 * @param circuit the circuit on which constant folding shall be applied
 */
void ConstantFolder::visit(core::CircuitObjectWrapper& circuit) {
    using op = core::ir::PrimitiveOperation;
    using pt = core::ir::PrimitiveType;
    for (auto node : circuit) {
        if (node.isInputNode() || node.isOutputNode() || node.isSubcircuitNode() || node.isLoopNode() || node.isNodeWithCustomOp()) {
            // there is nothing to do for these kind of nodes (yet) -> continue with the next node
            continue;
        }

        if (node.isConstantNode()) {
            // save the value of this constant and continue the evaluation
            auto flexbufferRef = node.getConstantFlexbuffer();
            constantNodes_[node.getNodeID()] = flexbufferRef;
            continue;
        }

        std::vector<flexbuffers::Reference> inputConstants;

        auto nodeInputs = node.getInputNodeIDs();

        if (node.usesInputOffsets()) {
            auto nodeOffsets = node.getInputOffsets();
            for (size_t it = 0, end = node.getNumberOfInputs(); it != end; ++it) {
                size_t inputNode = nodeInputs[it];
                size_t inputOffset = nodeOffsets[it];
                auto inputFB = circuit.getNodeWithID(inputNode).getConstantFlexbuffer();
                if (inputFB.IsAnyVector()) {
                    // use offset to access the vector's element and save that reference
                    inputConstants.push_back(inputFB.AsVector()[inputOffset]);
                } else {
                    // save the reference to the single data directly
                    inputConstants.push_back(inputFB);
                }
            }
        }

        // if node depends on one non-constant node, there is nothing to do
        // else, use this traversal to get the input constants for later
        bool containsOnlyConstants = true;
        for (auto input : nodeInputs) {
            if (!constantNodes_.contains(input)) {
                containsOnlyConstants = false;
                break;
            } else {
                auto inNode = circuit.getNodeWithID(input);
                inputConstants.push_back(inNode.getConstantFlexbuffer());
            }
        }
        // at least one input was not constant -> continue evaluation for the next node
        if (!containsOnlyConstants) {
            continue;
        }

        core::ir::PrimitiveType constantType = circuit.getNodeWithID(nodeInputs[0]).getConstantType().getPrimitiveType();

        // if node depends only on constant nodes: evaluate operation and mark it as a constant itself,
        // so that its children can treat it as a constant (fold)
        switch (node.getOperation()) {
            // accumulate:
            case op::And:
                visitBooleanAccumulation<core::PrimitiveOperationPolicies<op::And>>(constantType, node, inputConstants);
                break;
            case op::Xor:
                visitBooleanAccumulation<core::PrimitiveOperationPolicies<op::Xor>>(constantType, node, inputConstants);
                break;
            case op::Or:
                visitBooleanAccumulation<core::PrimitiveOperationPolicies<op::Or>>(constantType, node, inputConstants);
                break;
            case op::Add:
                visitArithmeticAccumulation<core::PrimitiveOperationPolicies<op::Add>>(constantType, node, inputConstants);
                break;
            case op::Mul:
                visitArithmeticAccumulation<core::PrimitiveOperationPolicies<op::Mul>>(constantType, node, inputConstants);
                break;
            case op::Div:
                visitArithmeticAccumulation<core::PrimitiveOperationPolicies<op::Div>>(constantType, node, inputConstants);
                break;
            case op::Sub:
                visitArithmeticAccumulation<core::PrimitiveOperationPolicies<op::Sub>>(constantType, node, inputConstants);
                break;
            // accumulate with underlying normal operation, then invert
            case op::Nand:
                visitAccumulateAndInvertOperation<core::PrimitiveOperationPolicies<op::And>>(constantType, node, inputConstants);
                break;
            case op::Nor:
                visitAccumulateAndInvertOperation<core::PrimitiveOperationPolicies<op::Or>>(constantType, node, inputConstants);
                break;
            case op::Xnor:
                visitAccumulateAndInvertOperation<core::PrimitiveOperationPolicies<op::Xor>>(constantType, node, inputConstants);
                break;
            // comparisons: always binary operands
            case op::Gt:
                visitComparisonOperation<core::PrimitiveOperationPolicies<op::Gt>>(constantType, node, inputConstants);
                break;
            case op::Ge:
                visitComparisonOperation<core::PrimitiveOperationPolicies<op::Ge>>(constantType, node, inputConstants);
                break;
            case op::Lt:
                visitComparisonOperation<core::PrimitiveOperationPolicies<op::Lt>>(constantType, node, inputConstants);
                break;
            case op::Le:
                visitComparisonOperation<core::PrimitiveOperationPolicies<op::Le>>(constantType, node, inputConstants);
                break;
            case op::Eq:
                visitComparisonOperation<core::PrimitiveOperationPolicies<op::Eq>>(constantType, node, inputConstants);
                break;
            case op::Neg:
                visitNegation(constantType, node, inputConstants);
                break;
            case op::Not:
                visitNot(constantType, node, inputConstants);
                break;
            case op::Mux:
                visitMux(constantType, node, inputConstants);
                break;
            case op::Split:
                visitSplit(constantType, node, inputConstants);
                break;
            case op::Merge:
                visitMerge(constantType, node, inputConstants);
                break;
            default:
                break;
        }

        // after visit: if this node is now a constant itself -> save the computed value
        if (node.isConstantNode()) {
            constantNodes_[node.getNodeID()] = node.getConstantFlexbuffer();
        }
    }
}

void foldConstantNodes(core::CircuitObjectWrapper& circuit) {
    ConstantFolder folder;
    folder.visit(circuit);
}

void foldConstantNodes(core::ModuleObjectWrapper& module) {
    auto circuitNames = module.getAllCircuitNames();
    for (auto name : circuitNames) {
        auto circuit = module.getCircuitWithName(name);
        foldConstantNodes(circuit);
    }
}

}  // namespace fuse::passes
