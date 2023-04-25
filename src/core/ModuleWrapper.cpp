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

#include "ModuleWrapper.h"

#include <iostream>
#include <list>
#include <regex>

#include "DOTBackend.h"
#include "NodeSuccessorsAnalysis.h"
#include "datatype_generated.h"

#include "NodeSuccessorsAnalysis.h"

#include <list>
#include <iostream>
#include "DOTBackend.h"

namespace fuse::core {

/*
 ****************************************** DataTypeBufferWrapper Member Functions ******************************************
 */

bool DataTypeBufferWrapper::isPrimitiveType() const { return (flatbuffers::IsFieldPresent(data_type_, data_type_->VT_SHAPE) || data_type_->shape()->Get(0) <= 1); }
bool DataTypeBufferWrapper::isSecureType() const { return data_type_->security_level() == core::ir::SecurityLevel::Secure; }
ir::PrimitiveType DataTypeBufferWrapper::getPrimitiveType() const { return data_type_->primitive_type(); }
std::string DataTypeBufferWrapper::getPrimitiveTypeName() const { return core::ir::EnumNamePrimitiveType(data_type_->primitive_type()); }
ir::SecurityLevel DataTypeBufferWrapper::getSecurityLevel() const { return data_type_->security_level(); }
std::string DataTypeBufferWrapper::getSecurityLevelName() const { return core::ir::EnumNameSecurityLevel(data_type_->security_level()); }

std::string DataTypeBufferWrapper::getDataTypeAnnotations() const {
    if (flatbuffers::IsFieldPresent(data_type_, data_type_->VT_DATA_TYPE_ANNOTATIONS)) {
        return data_type_->data_type_annotations()->str();
    } else {
        return "";
    }
}

std::string DataTypeBufferWrapper::getStringValueForAttribute(const std::string& attribute) const {
    std::regex attribute_regex(attribute + "\\s*:\\s*(\\w*)\\s*,?");
    std::smatch match;
    auto annotation_string = getDataTypeAnnotations();
    std::regex_search(annotation_string, match, attribute_regex);
    return match.str();
}

std::span<const int64_t>
DataTypeBufferWrapper::getShape() const {
    if (flatbuffers::IsFieldPresent(data_type_, data_type_->VT_SHAPE)) {
        return {data_type_->shape()->data(), data_type_->shape()->size()};
    } else {
        return {};
    }
}

/*
 ****************************************** DataTypeObjectWrapper Member Functions ******************************************
 */
bool DataTypeObjectWrapper::isPrimitiveType() const { return getShape().empty(); }
bool DataTypeObjectWrapper::isSecureType() const { return data_type_object_->security_level == core::ir::SecurityLevel::Secure; }
ir::PrimitiveType DataTypeObjectWrapper::getPrimitiveType() const { return data_type_object_->primitive_type; }
std::string DataTypeObjectWrapper::getPrimitiveTypeName() const { return core::ir::EnumNamePrimitiveType(data_type_object_->primitive_type); }
ir::SecurityLevel DataTypeObjectWrapper::getSecurityLevel() const { return data_type_object_->security_level; }
std::string DataTypeObjectWrapper::getSecurityLevelName() const { return core::ir::EnumNameSecurityLevel(data_type_object_->security_level); }
std::string DataTypeObjectWrapper::getDataTypeAnnotations() const { return data_type_object_->data_type_annotations; }
std::string DataTypeObjectWrapper::getStringValueForAttribute(const std::string& attribute) const {
    std::regex attribute_regex(attribute + "\\s*:\\s*(\\w*)\\s*,?");
    std::smatch match;
    auto annotation_string = getDataTypeAnnotations();
    std::regex_search(annotation_string, match, attribute_regex);
    return match.str();
}
std::span<const int64_t> DataTypeObjectWrapper::getShape() const { return {data_type_object_->shape.data(), data_type_object_->shape.size()}; }

void DataTypeObjectWrapper::setPrimitiveType(ir::PrimitiveType primitiveType) { data_type_object_->primitive_type = primitiveType; }
void DataTypeObjectWrapper::setSecurityLevel(ir::SecurityLevel securityLevel) { data_type_object_->security_level = securityLevel; }
void DataTypeObjectWrapper::setDataTypeAnnotations(const std::string& annotations) { data_type_object_->data_type_annotations = annotations; }
void DataTypeObjectWrapper::setStringValueForAttribute(const std::string& attribute, const std::string& value) {
    std::regex attribute_regex(attribute + "\\s*:\\s*(\\w*)\\s*,?");
    std::regex_replace(data_type_object_->data_type_annotations, attribute_regex, value);
}

void DataTypeObjectWrapper::setShape(std::span<const int64_t> shape) { data_type_object_->shape.assign(shape.begin(), shape.end()); }

/*
 ****************************************** NodeBufferWrapper Member Functions ******************************************
 */

uint64_t NodeBufferWrapper::getNodeID() const { return node_flatbuffer_->id(); }
ir::PrimitiveOperation NodeBufferWrapper::getOperation() const { return node_flatbuffer_->operation(); }
bool NodeBufferWrapper::isConstantNode() const { return getOperation() == ir::PrimitiveOperation::Constant; }
bool NodeBufferWrapper::isNodeWithCustomOp() const { return getOperation() == ir::PrimitiveOperation::Custom; }
bool NodeBufferWrapper::isSubcircuitNode() const { return getOperation() == ir::PrimitiveOperation::CallSubcircuit; }
bool NodeBufferWrapper::isLoopNode() const { return getOperation() == ir::PrimitiveOperation::Loop; }
bool NodeBufferWrapper::isSplitNode() const { return getOperation() == ir::PrimitiveOperation::Split; }
bool NodeBufferWrapper::isMergeNode() const { return getOperation() == ir::PrimitiveOperation::Merge; }
bool NodeBufferWrapper::isInputNode() const { return getOperation() == ir::PrimitiveOperation::Input; }
bool NodeBufferWrapper::isOutputNode() const { return getOperation() == ir::PrimitiveOperation::Output; }

bool NodeBufferWrapper::isUnaryNode() const {
    auto op = getOperation();
    return (op == ir::PrimitiveOperation::Neg) ||
           (op == ir::PrimitiveOperation::Not);
}

bool NodeBufferWrapper::isBinaryNode() const {
    auto op = getOperation();
    return (op == ir::PrimitiveOperation::And) ||
           (op == ir::PrimitiveOperation::Xor) ||
           (op == ir::PrimitiveOperation::Or) ||
           (op == ir::PrimitiveOperation::Nand) ||
           (op == ir::PrimitiveOperation::Nor) ||
           (op == ir::PrimitiveOperation::Xnor) ||
           (op == ir::PrimitiveOperation::Gt) ||
           (op == ir::PrimitiveOperation::Ge) ||
           (op == ir::PrimitiveOperation::Lt) ||
           (op == ir::PrimitiveOperation::Le) ||
           (op == ir::PrimitiveOperation::Eq) ||
           (op == ir::PrimitiveOperation::Add) ||
           (op == ir::PrimitiveOperation::Mul) ||
           (op == ir::PrimitiveOperation::Div) ||
           (op == ir::PrimitiveOperation::Sub);
}
bool NodeBufferWrapper::hasBooleanOperator() const {
    auto op = getOperation();
    return (op == ir::PrimitiveOperation::And) ||
           (op == ir::PrimitiveOperation::Xor) ||
           (op == ir::PrimitiveOperation::Not) ||
           (op == ir::PrimitiveOperation::Or) ||
           (op == ir::PrimitiveOperation::Nand) ||
           (op == ir::PrimitiveOperation::Nor) ||
           (op == ir::PrimitiveOperation::Xnor);
}

bool NodeBufferWrapper::hasComparisonOperator() const {
    auto op = getOperation();
    return (op == ir::PrimitiveOperation::Gt) ||
           (op == ir::PrimitiveOperation::Ge) ||
           (op == ir::PrimitiveOperation::Lt) ||
           (op == ir::PrimitiveOperation::Le) ||
           (op == ir::PrimitiveOperation::Eq);
}

bool NodeBufferWrapper::hasArithmeticOperator() const {
    auto op = getOperation();
    return (op == ir::PrimitiveOperation::Add) ||
           (op == ir::PrimitiveOperation::Mul) ||
           (op == ir::PrimitiveOperation::Div) ||
           (op == ir::PrimitiveOperation::Neg) ||
           (op == ir::PrimitiveOperation::Sub);
}

bool NodeBufferWrapper::usesInputOffsets() const { return flatbuffers::IsFieldPresent(node_flatbuffer_, node_flatbuffer_->VT_INPUT_OFFSETS); }

std::string NodeBufferWrapper::getOperationName() const { return core::ir::EnumNamePrimitiveOperation(node_flatbuffer_->operation()); }
std::string NodeBufferWrapper::getCustomOperationName() const { return node_flatbuffer_->custom_op_name()->str(); }

std::string NodeBufferWrapper::getSubCircuitName() const { return node_flatbuffer_->subcircuit_name()->str(); }

std::string NodeBufferWrapper::getNodeAnnotations() const {
    if (flatbuffers::IsFieldPresent(node_flatbuffer_, node_flatbuffer_->VT_NODE_ANNOTATIONS)) {
        return node_flatbuffer_->node_annotations()->str();
    } else {
        return "";
    }
}

std::string NodeBufferWrapper::getStringValueForAttribute(std::string attribute) const {
    std::regex attribute_regex(attribute + "\\s*:\\s*(\\w*)\\s*,?");
    std::smatch match;
    auto annotation_string = getNodeAnnotations();
    std::regex_search(annotation_string, match, attribute_regex);
    return match.str();
}

std::span<const uint64_t> NodeBufferWrapper::getInputNodeIDs() const {
    if (flatbuffers::IsFieldPresent(node_flatbuffer_, node_flatbuffer_->VT_INPUT_IDENTIFIERS)) {
        return {node_flatbuffer_->input_identifiers()->data(), node_flatbuffer_->input_identifiers()->size()};
    } else {
        return {};
    }
}

std::span<const uint32_t> NodeBufferWrapper::getInputOffsets() const {
    if (flatbuffers::IsFieldPresent(node_flatbuffer_, node_flatbuffer_->VT_INPUT_OFFSETS)) {
        return {node_flatbuffer_->input_offsets()->data(), node_flatbuffer_->input_offsets()->size()};
    } else {
        return {};
    }
}

NodeBufferWrapper::DataType NodeBufferWrapper::getInputDataTypeAt(size_t inputNumber) const {
    if (inputNumber < getNumberOfInputs()) {
        return std::make_unique<DataTypeBufferWrapper>(node_flatbuffer_->input_datatypes()->Get(inputNumber));
    } else {
        throw std::invalid_argument("invalid input number: " + std::to_string(inputNumber) + " for node with ID: " + std::to_string(getNodeID()) + "\n");
    }
}

std::vector<NodeBufferWrapper::DataType> NodeBufferWrapper::getInputDataTypes() const {
    if (flatbuffers::IsFieldPresent(node_flatbuffer_, node_flatbuffer_->VT_INPUT_DATATYPES)) {
        std::vector<NodeBufferWrapper::DataType> datatypeBufferWrappers;
        for (auto it = node_flatbuffer_->input_datatypes()->begin(), end = node_flatbuffer_->input_datatypes()->end(); it != end; ++it) {
            datatypeBufferWrappers.push_back(std::make_unique<DataTypeBufferWrapper>(*it));
        }
        return datatypeBufferWrappers;
    } else {
        return {};
    }
}

size_t NodeBufferWrapper::getNumberOfInputs() const {
    if (flatbuffers::IsFieldPresent(node_flatbuffer_, node_flatbuffer_->VT_INPUT_IDENTIFIERS)) {
        return node_flatbuffer_->input_identifiers()->size();
    } else {
        return 0;
    }
}

NodeBufferWrapper::DataType NodeBufferWrapper::getOutputDataTypeAt(size_t outputNumber) const {
    if (outputNumber < getNumberOfOutputs()) {
        return std::make_unique<DataTypeBufferWrapper>(node_flatbuffer_->output_datatypes()->Get(outputNumber));
    } else {
        throw std::invalid_argument("invalid input number: " + std::to_string(outputNumber) + " for node with ID: " + std::to_string(getNodeID()) + "\n");
    }
}

std::vector<NodeBufferWrapper::DataType> NodeBufferWrapper::getOutputDataTypes() const {
    if (flatbuffers::IsFieldPresent(node_flatbuffer_, node_flatbuffer_->VT_OUTPUT_DATATYPES)) {
        std::vector<NodeBufferWrapper::DataType> datatypeBufferWrappers;
        for (auto it = node_flatbuffer_->output_datatypes()->begin(), end = node_flatbuffer_->output_datatypes()->end(); it != end; ++it) {
            datatypeBufferWrappers.push_back(std::make_unique<DataTypeBufferWrapper>(*it));
        }
        return datatypeBufferWrappers;
    } else {
        return {};
    }
}

size_t NodeBufferWrapper::getNumberOfOutputs() const { return node_flatbuffer_->num_of_outputs(); }

NodeBufferWrapper::DataType NodeBufferWrapper::getConstantType() const {
    assert(node_flatbuffer_->output_datatypes()->size() == 1);
    return std::make_unique<DataTypeBufferWrapper>(node_flatbuffer_->output_datatypes()->Get(0));
}

const flexbuffers::Reference NodeBufferWrapper::getConstantFlexbuffer() const { return node_flatbuffer_->payload_flexbuffer_root(); }
bool NodeBufferWrapper::getConstantBool() const { return node_flatbuffer_->payload_flexbuffer_root().AsBool(); }
int8_t NodeBufferWrapper::getConstantInt8() const { return node_flatbuffer_->payload_flexbuffer_root().AsInt8(); }
int16_t NodeBufferWrapper::getConstantInt16() const { return node_flatbuffer_->payload_flexbuffer_root().AsInt16(); }
int32_t NodeBufferWrapper::getConstantInt32() const { return node_flatbuffer_->payload_flexbuffer_root().AsInt32(); }
int64_t NodeBufferWrapper::getConstantInt64() const { return node_flatbuffer_->payload_flexbuffer_root().AsInt64(); }
uint8_t NodeBufferWrapper::getConstantUInt8() const { return node_flatbuffer_->payload_flexbuffer_root().AsUInt8(); }
uint16_t NodeBufferWrapper::getConstantUInt16() const { return node_flatbuffer_->payload_flexbuffer_root().AsUInt16(); }
uint32_t NodeBufferWrapper::getConstantUInt32() const { return node_flatbuffer_->payload_flexbuffer_root().AsUInt32(); }
uint64_t NodeBufferWrapper::getConstantUInt64() const { return node_flatbuffer_->payload_flexbuffer_root().AsUInt64(); }
float NodeBufferWrapper::getConstantFloat() const { return node_flatbuffer_->payload_flexbuffer_root().AsFloat(); }
double NodeBufferWrapper::getConstantDouble() const { return node_flatbuffer_->payload_flexbuffer_root().AsDouble(); }

std::span<const uint8_t> NodeBufferWrapper::getConstantBinary() const {
    auto blob = node_flatbuffer_->payload_flexbuffer_root().AsBlob();
    return {blob.data(), blob.read_size()};
}

template <typename Value>
std::vector<Value> NodeBufferWrapper::getConstantVector() const {
    auto flexbuffer_vector = node_flatbuffer_->payload_flexbuffer_root().AsVector();
    size_t size = flexbuffer_vector.read_size();
    std::vector<Value> result;
    result.reserve(size);
    for (size_t it = 0; it < size; ++it) {
        result.push_back(flexbuffer_vector[it].As<Value>());
    }
    return result;
}

std::vector<bool> NodeBufferWrapper::getConstantBoolVector() const { return getConstantVector<bool>(); }
std::vector<int8_t> NodeBufferWrapper::getConstantInt8Vector() const { return getConstantVector<int8_t>(); }
std::vector<int16_t> NodeBufferWrapper::getConstantInt16Vector() const { return getConstantVector<int16_t>(); }
std::vector<int32_t> NodeBufferWrapper::getConstantInt32Vector() const { return getConstantVector<int32_t>(); }
std::vector<int64_t> NodeBufferWrapper::getConstantInt64Vector() const { return getConstantVector<int64_t>(); }
std::vector<uint8_t> NodeBufferWrapper::getConstantUInt8Vector() const { return getConstantVector<uint8_t>(); }
std::vector<uint16_t> NodeBufferWrapper::getConstantUInt16Vector() const { return getConstantVector<uint16_t>(); }
std::vector<uint32_t> NodeBufferWrapper::getConstantUInt32Vector() const { return getConstantVector<uint32_t>(); }
std::vector<uint64_t> NodeBufferWrapper::getConstantUInt64Vector() const { return getConstantVector<uint64_t>(); }
std::vector<float> NodeBufferWrapper::getConstantFloatVector() const { return getConstantVector<float>(); }
std::vector<double> NodeBufferWrapper::getConstantDoubleVector() const { return getConstantVector<double>(); }

std::vector<std::vector<bool>> NodeBufferWrapper::getConstantBoolMatrix() const {
    // get matrix
    auto fb_matrix = node_flatbuffer_->payload_flexbuffer_root().AsVector();
    size_t size = fb_matrix.read_size();
    std::vector<std::vector<bool>> result;
    result.reserve(size);
    // iterate over vectors inside matrix
    for (size_t it = 0; it < size; ++it) {
        auto fb_vec = fb_matrix[it].AsVector();
        std::vector<bool> res_vec;
        size_t vec_size = fb_vec.read_size();
        res_vec.reserve(vec_size);
        // read out values inside vector
        for (size_t j = 0; j < vec_size; ++j) {
            res_vec.push_back(fb_vec[j].AsBool());
        }
        // write the vector into the matrix
        result.push_back(res_vec);
    }
    return result;
}

/*
 ****************************************** NodeObjectWrapper Member Functions ******************************************
 */

bool NodeObjectWrapper::isConstantNode() const { return getOperation() == ir::PrimitiveOperation::Constant; }
bool NodeObjectWrapper::isNodeWithCustomOp() const { return getOperation() == ir::PrimitiveOperation::Custom; }
bool NodeObjectWrapper::isSubcircuitNode() const { return getOperation() == ir::PrimitiveOperation::CallSubcircuit; }
bool NodeObjectWrapper::isLoopNode() const { return getOperation() == ir::PrimitiveOperation::Loop; }
bool NodeObjectWrapper::isSplitNode() const { return getOperation() == ir::PrimitiveOperation::Split; }
bool NodeObjectWrapper::isMergeNode() const { return getOperation() == ir::PrimitiveOperation::Merge; }
bool NodeObjectWrapper::isInputNode() const { return getOperation() == ir::PrimitiveOperation::Input; }
bool NodeObjectWrapper::isOutputNode() const { return getOperation() == ir::PrimitiveOperation::Output; }
bool NodeObjectWrapper::isUnaryNode() const {
    auto op = getOperation();
    return (op == ir::PrimitiveOperation::Neg) ||
           (op == ir::PrimitiveOperation::Not);
}
bool NodeObjectWrapper::isBinaryNode() const {
    auto op = getOperation();
    return (op == ir::PrimitiveOperation::And) ||
           (op == ir::PrimitiveOperation::Xor) ||
           (op == ir::PrimitiveOperation::Or) ||
           (op == ir::PrimitiveOperation::Nand) ||
           (op == ir::PrimitiveOperation::Nor) ||
           (op == ir::PrimitiveOperation::Xnor) ||
           (op == ir::PrimitiveOperation::Gt) ||
           (op == ir::PrimitiveOperation::Ge) ||
           (op == ir::PrimitiveOperation::Lt) ||
           (op == ir::PrimitiveOperation::Le) ||
           (op == ir::PrimitiveOperation::Eq) ||
           (op == ir::PrimitiveOperation::Add) ||
           (op == ir::PrimitiveOperation::Mul) ||
           (op == ir::PrimitiveOperation::Div) ||
           (op == ir::PrimitiveOperation::Sub);
}
bool NodeObjectWrapper::hasBooleanOperator() const {
    auto op = getOperation();
    return (op == ir::PrimitiveOperation::And) ||
           (op == ir::PrimitiveOperation::Xor) ||
           (op == ir::PrimitiveOperation::Not) ||
           (op == ir::PrimitiveOperation::Or) ||
           (op == ir::PrimitiveOperation::Nand) ||
           (op == ir::PrimitiveOperation::Nor) ||
           (op == ir::PrimitiveOperation::Xnor);
}
bool NodeObjectWrapper::hasComparisonOperator() const {
    auto op = getOperation();
    return (op == ir::PrimitiveOperation::Gt) ||
           (op == ir::PrimitiveOperation::Ge) ||
           (op == ir::PrimitiveOperation::Lt) ||
           (op == ir::PrimitiveOperation::Le) ||
           (op == ir::PrimitiveOperation::Eq);
}
bool NodeObjectWrapper::hasArithmeticOperator() const {
    auto op = getOperation();
    return (op == ir::PrimitiveOperation::Add) ||
           (op == ir::PrimitiveOperation::Mul) ||
           (op == ir::PrimitiveOperation::Div) ||
           (op == ir::PrimitiveOperation::Neg) ||
           (op == ir::PrimitiveOperation::Sub);
}

bool NodeObjectWrapper::usesInputOffsets() const { return !node_object_->input_offsets.empty(); }
uint64_t NodeObjectWrapper::getNodeID() const { return node_object_->id; }
ir::PrimitiveOperation NodeObjectWrapper::getOperation() const { return node_object_->operation; }
std::string NodeObjectWrapper::getOperationName() const { return core::ir::EnumNamePrimitiveOperation(node_object_->operation); }
std::string NodeObjectWrapper::getCustomOperationName() const { return node_object_->custom_op_name; }
std::string NodeObjectWrapper::getSubCircuitName() const { return node_object_->subcircuit_name; }
std::string NodeObjectWrapper::getNodeAnnotations() const { return node_object_->node_annotations; }
std::string NodeObjectWrapper::getStringValueForAttribute(std::string attribute) const {
    std::regex attribute_regex(attribute + "\\s*:\\s*(\\w*)\\s*,?");
    std::smatch match;
    auto annotation_string = getNodeAnnotations();
    std::regex_search(annotation_string, match, attribute_regex);
    return match.str();
}

std::span<const uint64_t> NodeObjectWrapper::getInputNodeIDs() const {
    if (node_object_->input_identifiers.empty()) {
        return {};
    } else {
        return {node_object_->input_identifiers.data(), node_object_->input_identifiers.size()};
    }
}
std::span<const uint32_t> NodeObjectWrapper::getInputOffsets() const {
    if (node_object_->input_offsets.empty()) {
        return {};
    } else {
        return {node_object_->input_offsets.data(), node_object_->input_offsets.size()};
    }
}

NodeObjectWrapper::DataType NodeObjectWrapper::getInputDataTypeAt(size_t inputNumber) const {
    if (inputNumber < getNumberOfInputs()) {
        return std::make_unique<DataTypeObjectWrapper>(node_object_->input_datatypes.at(inputNumber).get());
    } else {
        throw std::invalid_argument("invalid input number: " + std::to_string(inputNumber) + " for node with ID: " + std::to_string(getNodeID()) + "\n");
    }
}

std::vector<NodeObjectWrapper::DataType> NodeObjectWrapper::getInputDataTypes() const {
    std::vector<NodeObjectWrapper::DataType> types;
    for (auto& it : node_object_->input_datatypes) {
        types.push_back(std::make_unique<DataTypeObjectWrapper>(it.get()));
    }
    return types;
}

size_t NodeObjectWrapper::getNumberOfInputs() const { return node_object_->input_identifiers.size(); }

NodeObjectWrapper::DataType NodeObjectWrapper::getOutputDataTypeAt(size_t inputNumber) const {
    if (inputNumber < getNumberOfOutputs()) {
        return std::make_unique<DataTypeObjectWrapper>(node_object_->output_datatypes.at(inputNumber).get());
    } else {
        throw std::invalid_argument("invalid input number: " + std::to_string(inputNumber) + " for node with ID: " + std::to_string(getNodeID()) + "\n");
    }
}
std::vector<NodeObjectWrapper::DataType> NodeObjectWrapper::getOutputDataTypes() const {
    std::vector<NodeObjectWrapper::DataType> types;
    for (auto& it : node_object_->output_datatypes) {
        types.push_back(std::make_unique<DataTypeObjectWrapper>(it.get()));
    }
    return types;
}
size_t NodeObjectWrapper::getNumberOfOutputs() const { return node_object_->num_of_outputs; }

NodeObjectWrapper::DataType NodeObjectWrapper::getConstantType() const {
    assert(node_object_->output_datatypes.size() == 1);
    return std::make_unique<DataTypeObjectWrapper>(node_object_->output_datatypes.at(0).get());
}

// whoever feels like it, may also access the flexbuffer directly
const flexbuffers::Reference NodeObjectWrapper::getConstantFlexbuffer() const { return flexbuffers::GetRoot(node_object_->payload); }
bool NodeObjectWrapper::getConstantBool() const { return flexbuffers::GetRoot(node_object_->payload).AsBool(); }
int8_t NodeObjectWrapper::getConstantInt8() const { return flexbuffers::GetRoot(node_object_->payload).AsInt8(); }
int16_t NodeObjectWrapper::getConstantInt16() const { return flexbuffers::GetRoot(node_object_->payload).AsInt16(); }
int32_t NodeObjectWrapper::getConstantInt32() const { return flexbuffers::GetRoot(node_object_->payload).AsInt32(); }
int64_t NodeObjectWrapper::getConstantInt64() const { return flexbuffers::GetRoot(node_object_->payload).AsInt64(); }
uint8_t NodeObjectWrapper::getConstantUInt8() const { return flexbuffers::GetRoot(node_object_->payload).AsUInt8(); }
uint16_t NodeObjectWrapper::getConstantUInt16() const { return flexbuffers::GetRoot(node_object_->payload).AsUInt16(); }
uint32_t NodeObjectWrapper::getConstantUInt32() const { return flexbuffers::GetRoot(node_object_->payload).AsUInt32(); }
uint64_t NodeObjectWrapper::getConstantUInt64() const { return flexbuffers::GetRoot(node_object_->payload).AsUInt64(); }
float NodeObjectWrapper::getConstantFloat() const { return flexbuffers::GetRoot(node_object_->payload).AsFloat(); }
double NodeObjectWrapper::getConstantDouble() const { return flexbuffers::GetRoot(node_object_->payload).AsDouble(); }

std::span<const uint8_t> NodeObjectWrapper::getConstantBinary() const {
    auto blob = flexbuffers::GetRoot(node_object_->payload).AsBlob();
    return {blob.data(), blob.read_size()};
}

template <typename Value>
std::vector<Value> NodeObjectWrapper::getConstantVector() const {
    auto flexbuffer_vector = flexbuffers::GetRoot(node_object_->payload).AsVector();
    size_t size = flexbuffer_vector.read_size();
    std::vector<Value> result(size);
    for (size_t it = 0; it < size; ++it) {
        result.push_back(flexbuffer_vector[it].As<Value>());
    }
    return result;
}

std::vector<bool> NodeObjectWrapper::getConstantBoolVector() const { return getConstantVector<bool>(); }
std::vector<int8_t> NodeObjectWrapper::getConstantInt8Vector() const { return getConstantVector<int8_t>(); }
std::vector<int16_t> NodeObjectWrapper::getConstantInt16Vector() const { return getConstantVector<int16_t>(); }
std::vector<int32_t> NodeObjectWrapper::getConstantInt32Vector() const { return getConstantVector<int32_t>(); }
std::vector<int64_t> NodeObjectWrapper::getConstantInt64Vector() const { return getConstantVector<int64_t>(); }
std::vector<uint8_t> NodeObjectWrapper::getConstantUInt8Vector() const { return getConstantVector<uint8_t>(); }
std::vector<uint16_t> NodeObjectWrapper::getConstantUInt16Vector() const { return getConstantVector<uint16_t>(); }
std::vector<uint32_t> NodeObjectWrapper::getConstantUInt32Vector() const { return getConstantVector<uint32_t>(); }
std::vector<uint64_t> NodeObjectWrapper::getConstantUInt64Vector() const { return getConstantVector<uint64_t>(); }
std::vector<float> NodeObjectWrapper::getConstantFloatVector() const { return getConstantVector<float>(); }
std::vector<double> NodeObjectWrapper::getConstantDoubleVector() const { return getConstantVector<double>(); }

std::vector<std::vector<bool>> NodeObjectWrapper::getConstantBoolMatrix() const {
    // get matrix
    auto fb_matrix = flexbuffers::GetRoot(node_object_->payload).AsVector();
    size_t size = fb_matrix.read_size();
    std::vector<std::vector<bool>> res_mat;
    res_mat.reserve(size);
    // iterate over vectors inside matrix
    for (size_t it = 0; it < size; ++it) {
        auto fb_vec = fb_matrix[it].AsVector();
        std::vector<bool> res_vec;
        size_t vec_size = fb_vec.read_size();
        res_vec.reserve(vec_size);
        // read out values inside vector
        for (size_t j = 0; j < vec_size; ++j) {
            res_vec.push_back(fb_vec[j].AsBool());
        }
        // write the vector into the matrix
        res_mat.push_back(res_vec);
    }
    return res_mat;
}

void NodeObjectWrapper::setNodeID(uint64_t nodeID) { node_object_->id = nodeID; }
void NodeObjectWrapper::setPrimitiveOperation(ir::PrimitiveOperation primitiveOperation) { node_object_->operation = primitiveOperation; }
void NodeObjectWrapper::setCustomOperationName(const std::string& customOperationName) { node_object_->custom_op_name = customOperationName; }
void NodeObjectWrapper::setSubCircuitName(const std::string& subcircuitName) { node_object_->subcircuit_name = subcircuitName; }
void NodeObjectWrapper::setNodeAnnotations(const std::string& nodeAnnotations) { node_object_->node_annotations = nodeAnnotations; }
void NodeObjectWrapper::setStringValueForAttribute(const std::string& attribute, const std::string& value) {
    std::regex attribute_regex(attribute + "\\s*:\\s*(\\w*)\\s*,?");
    std::regex_replace(node_object_->node_annotations, attribute_regex, value);
}

void NodeObjectWrapper::setConstantType(ir::PrimitiveType primitiveType, std::span<const int64_t> shape) {
    node_object_->input_datatypes.clear();
    node_object_->output_datatypes.clear();
    auto dt = std::make_unique<ir::DataTypeTableT>();
    dt->security_level = ir::SecurityLevel::Plaintext;
    dt->primitive_type = primitiveType;
    dt->shape.assign(shape.begin(), shape.end());
    node_object_->output_datatypes.push_back(std::move(dt));
}

void NodeObjectWrapper::setInputNodeIDs(std::span<uint64_t> inputNodeIDs) { node_object_->input_identifiers.assign(inputNodeIDs.begin(), inputNodeIDs.end()); }
void NodeObjectWrapper::setInputOffsets(std::span<uint32_t> inputOffsets) { node_object_->input_offsets.assign(inputOffsets.begin(), inputOffsets.end()); }
void NodeObjectWrapper::setNumberOfOutputs(uint32_t numberOfOutputs) { node_object_->num_of_outputs = numberOfOutputs; }
void NodeObjectWrapper::setPayload(const std::vector<uint8_t>& finishedFlexbuffer) { node_object_->payload = finishedFlexbuffer; }
void NodeObjectWrapper::setPayload(bool boolValue) {
    flexbuffers::Builder fbb;
    fbb.Bool(boolValue);
    fbb.Finish();
    node_object_->payload = fbb.GetBuffer();
}
void NodeObjectWrapper::setPayload(uint64_t unsignedValue) {
    flexbuffers::Builder fbb;
    fbb.UInt(unsignedValue);
    fbb.Finish();
    node_object_->payload = fbb.GetBuffer();
}
void NodeObjectWrapper::setPayload(int64_t intValue) {
    flexbuffers::Builder fbb;
    fbb.Int(intValue);
    fbb.Finish();
    node_object_->payload = fbb.GetBuffer();
}
void NodeObjectWrapper::setPayload(float floatValue) {
    flexbuffers::Builder fbb;
    fbb.Float(floatValue);
    fbb.Finish();
    node_object_->payload = fbb.GetBuffer();
}
void NodeObjectWrapper::setPayload(double doubleValue) {
    flexbuffers::Builder fbb;
    fbb.Double(doubleValue);
    fbb.Finish();
    node_object_->payload = fbb.GetBuffer();
}
void NodeObjectWrapper::setPayload(const std::vector<bool>& boolVector) {
    flexbuffers::Builder fbb;
    auto start = fbb.StartVector();
    for (auto val : boolVector) {
        fbb.Bool(val);
    }
    fbb.EndVector(start, true, true);
    fbb.Finish();
    node_object_->payload = fbb.GetBuffer();
}
void NodeObjectWrapper::setPayload(const std::vector<uint64_t>& unsignedVector) {
    flexbuffers::Builder fbb;
    auto start = fbb.StartVector();
    for (auto val : unsignedVector) {
        fbb.UInt(val);
    }
    fbb.EndVector(start, true, true);
    fbb.Finish();
    node_object_->payload = fbb.GetBuffer();
}
void NodeObjectWrapper::setPayload(const std::vector<int64_t>& intVector) {
    flexbuffers::Builder fbb;
    auto start = fbb.StartVector();
    for (auto val : intVector) {
        fbb.Int(val);
    }
    fbb.EndVector(start, true, true);
    fbb.Finish();
    node_object_->payload = fbb.GetBuffer();
}
void NodeObjectWrapper::setPayload(const std::vector<float>& floatVector) {
    flexbuffers::Builder fbb;
    auto start = fbb.StartVector();
    for (auto val : floatVector) {
        fbb.Float(val);
    }
    fbb.EndVector(start, true, true);
    fbb.Finish();
    node_object_->payload = fbb.GetBuffer();
}
void NodeObjectWrapper::setPayload(const std::vector<double>& doubleVector) {
    flexbuffers::Builder fbb;
    auto start = fbb.StartVector();
    for (auto val : doubleVector) {
        fbb.Double(val);
    }
    fbb.EndVector(start, true, true);
    fbb.Finish();
    node_object_->payload = fbb.GetBuffer();
}

NodeObjectWrapper::MutableDataType NodeObjectWrapper::getInputDataTypeAt(size_t inputNumber) {
    if (inputNumber < getNumberOfInputs()) {
        return DataTypeObjectWrapper(node_object_->input_datatypes.at(inputNumber).get());
    } else {
        throw std::invalid_argument("invalid input number: " + std::to_string(inputNumber) + " for node with ID: " + std::to_string(getNodeID()) + "\n");
    }
}
std::vector<NodeObjectWrapper::MutableDataType> NodeObjectWrapper::getInputDataTypes() {
    std::vector<NodeObjectWrapper::MutableDataType> types;
    for (auto& it : node_object_->input_datatypes) {
        types.push_back(DataTypeObjectWrapper(it.get()));
    }
    return types;
}
NodeObjectWrapper::MutableDataType NodeObjectWrapper::getOutputDataTypeAt(size_t inputNumber) {
    if (inputNumber < getNumberOfOutputs()) {
        return DataTypeObjectWrapper(node_object_->output_datatypes.at(inputNumber).get());
    } else {
        throw std::invalid_argument("invalid input number: " + std::to_string(inputNumber) + " for node with ID: " + std::to_string(getNodeID()) + "\n");
    }
}
std::vector<NodeObjectWrapper::MutableDataType> NodeObjectWrapper::getOutputDataTypes() {
    std::vector<NodeObjectWrapper::MutableDataType> types;
    for (auto& it : node_object_->output_datatypes) {
        types.push_back(DataTypeObjectWrapper(it.get()));
    }
    return types;
}
NodeObjectWrapper::MutableDataType NodeObjectWrapper::getConstantType() {
    assert(node_object_->output_datatypes.size() == 1);
    return DataTypeObjectWrapper(node_object_->output_datatypes.at(0).get());
}

void NodeObjectWrapper::replaceInputBy(uint64_t prevInputID, uint64_t newInputID, uint32_t prevOffset, uint32_t newOffset) {
    // add offsets to the node if not already existing to refer to the correct output
    if (node_object_->input_offsets.empty()) {
        auto it = node_object_->input_offsets.begin();
        node_object_->input_offsets.insert(it, getNumberOfInputs(), 0);
    }

    for (int i = 0; i < getNumberOfInputs(); ++i) {
        if (getInputNodeIDs()[i] == prevInputID) {
            // if (newOffset != prevOffset) {
            //     if (getInputOffsets()[i] == prevOffset) {
            node_object_->input_offsets[i] = newOffset;
            //     }
            // }
            node_object_->input_identifiers[i] = newInputID;
        }
    }
}

/*
 ****************************************** CircuitBufferWrapper Member Functions ******************************************
 */

std::string CircuitBufferWrapper::getName() const { return circuit_flatbuffer_->name()->str(); }

std::string CircuitBufferWrapper::getCircuitAnnotations() const { return circuit_flatbuffer_->circuit_annotations()->str(); }

std::string CircuitBufferWrapper::getStringValueForAttribute(const std::string& attribute) const {
    std::regex attribute_regex(attribute + "\\s*:\\s*(\\w*)\\s*,?");
    std::smatch match;
    auto annotation_string = getCircuitAnnotations();
    std::regex_search(annotation_string, match, attribute_regex);
    return match.str();
}

std::span<const uint64_t> CircuitBufferWrapper::getInputNodeIDs() const { return {circuit_flatbuffer_->inputs()->data(), circuit_flatbuffer_->inputs()->size()}; }

std::vector<CircuitBufferWrapper::DataType> CircuitBufferWrapper::getInputDataTypes() const {
    std::vector<CircuitBufferWrapper::DataType> datatypeBufferWrappers;
    for (auto it = circuit_flatbuffer_->input_datatypes()->begin(), end = circuit_flatbuffer_->input_datatypes()->end(); it != end; ++it) {
        datatypeBufferWrappers.push_back(std::make_unique<DataTypeBufferWrapper>(*it));
    }
    return datatypeBufferWrappers;
}
size_t CircuitBufferWrapper::getNumberOfInputs() const { return circuit_flatbuffer_->inputs()->size(); }

std::span<const uint64_t> CircuitBufferWrapper::getOutputNodeIDs() const { return {circuit_flatbuffer_->outputs()->data(), circuit_flatbuffer_->outputs()->size()}; }

std::vector<CircuitBufferWrapper::DataType> CircuitBufferWrapper::getOutputDataTypes() const {
    std::vector<CircuitBufferWrapper::DataType> datatypeBufferWrappers;
    for (auto it = circuit_flatbuffer_->output_datatypes()->begin(), end = circuit_flatbuffer_->output_datatypes()->end(); it != end; ++it) {
        datatypeBufferWrappers.push_back(std::make_unique<DataTypeBufferWrapper>(*it));
    }
    return datatypeBufferWrappers;
}

size_t CircuitBufferWrapper::getNumberOfOutputs() const { return circuit_flatbuffer_->outputs()->size(); }

CircuitBufferWrapper::Node CircuitBufferWrapper::getNodeWithID(uint64_t nodeID) const {
    // TODO this is not working when the vector is not sorted
    // return NodeBufferWrapper(circuit_flatbuffer_->nodes()->LookupByKey(nodeID));
    for (auto it = circuit_flatbuffer_->nodes()->begin(), end = circuit_flatbuffer_->nodes()->end(); it != end; ++it) {
        if (nodeID == (*it)->id()) {
            return std::make_unique<NodeBufferWrapper>(*it);
        }
    }
    throw std::runtime_error("Node could not be found with ID: " + nodeID);
}

size_t CircuitBufferWrapper::getNumberOfNodes() const { return circuit_flatbuffer_->nodes()->size(); }

/*
 ****************************************** CircuitObjectWrapper Member Functions ******************************************
 */

std::string CircuitObjectWrapper::getName() const { return circuit_object_->name; }

std::string CircuitObjectWrapper::getCircuitAnnotations() const { return circuit_object_->circuit_annotations; }

std::string CircuitObjectWrapper::getStringValueForAttribute(const std::string& attribute) const {
    std::regex attribute_regex(attribute + "\\s*:\\s*(\\w*)\\s*,?");
    std::smatch match;
    auto annotation_string = getCircuitAnnotations();
    std::regex_search(annotation_string, match, attribute_regex);
    return match.str();
}

std::span<const uint64_t> CircuitObjectWrapper::getInputNodeIDs() const { return {circuit_object_->inputs.data(), circuit_object_->inputs.size()}; }

std::vector<CircuitObjectWrapper::DataType> CircuitObjectWrapper::getInputDataTypes() const {
    std::vector<CircuitObjectWrapper::DataType> types;
    for (auto& it : circuit_object_->input_datatypes) {
        types.push_back(std::make_unique<DataTypeObjectWrapper>(it.get()));
    }
    return types;
}

size_t CircuitObjectWrapper::getNumberOfInputs() const { return circuit_object_->inputs.size(); }

std::span<const uint64_t> CircuitObjectWrapper::getOutputNodeIDs() const { return {circuit_object_->outputs.data(), circuit_object_->outputs.size()}; }

std::vector<CircuitObjectWrapper::DataType> CircuitObjectWrapper::getOutputDataTypes() const {
    std::vector<CircuitObjectWrapper::DataType> types;
    for (auto& it : circuit_object_->input_datatypes) {
        types.push_back(std::make_unique<DataTypeObjectWrapper>(it.get()));
    }
    return types;
}

size_t CircuitObjectWrapper::getNumberOfOutputs() const { return circuit_object_->outputs.size(); }

CircuitObjectWrapper::Node CircuitObjectWrapper::getNodeWithID(uint64_t nodeID) const {
    for (auto& node : circuit_object_->nodes) {
        if (node->id == nodeID) {
            return std::make_unique<NodeObjectWrapper>(node.get());
        }
    }
    throw std::runtime_error("Node could not be found with ID: " + nodeID);
}

size_t CircuitObjectWrapper::getNumberOfNodes() const { return circuit_object_->nodes.size(); }

void CircuitObjectWrapper::setName(const std::string& name) { circuit_object_->name = name; }

void CircuitObjectWrapper::setCircuitAnnotations(const std::string& annotations) { circuit_object_->circuit_annotations = annotations; }

void CircuitObjectWrapper::setStringValueForAttribute(const std::string& attribute, const std::string& value) {
    std::regex attribute_regex(attribute + "\\s*:\\s*(\\w*)\\s*,?");
    std::regex_replace(circuit_object_->circuit_annotations, attribute_regex, value);
}

CircuitObjectWrapper::MutableDataType CircuitObjectWrapper::getInputDataTypeAt(size_t inputNumber) {
    if (inputNumber < getNumberOfInputs()) {
        return DataTypeObjectWrapper(circuit_object_->input_datatypes.at(inputNumber).get());
    } else {
        throw std::invalid_argument("invalid input datatype number: " + std::to_string(inputNumber) + "\n");
    }
}

std::vector<CircuitObjectWrapper::MutableDataType> CircuitObjectWrapper::getInputDataTypes() {
    std::vector<CircuitObjectWrapper::MutableDataType> types;
    for (auto& it : circuit_object_->input_datatypes) {
        types.push_back(DataTypeObjectWrapper(it.get()));
    }
    return types;
}

void CircuitObjectWrapper::setInputNodeIDs(std::span<uint64_t> inputNodeIDs) { circuit_object_->inputs.assign(inputNodeIDs.begin(), inputNodeIDs.end()); }

CircuitObjectWrapper::MutableDataType CircuitObjectWrapper::getOutputDataTypeAt(size_t inputNumber) {
    if (inputNumber < getNumberOfOutputs()) {
        return DataTypeObjectWrapper(circuit_object_->output_datatypes.at(inputNumber).get());
    } else {
        throw std::invalid_argument("invalid input datatype number: " + std::to_string(inputNumber) + "\n");
    }
}

std::vector<CircuitObjectWrapper::MutableDataType> CircuitObjectWrapper::getOutputDataTypes() {
    std::vector<CircuitObjectWrapper::MutableDataType> types;
    for (auto& it : circuit_object_->output_datatypes) {
        types.push_back(DataTypeObjectWrapper(it.get()));
    }
    return types;
}

void CircuitObjectWrapper::setOutputNodeIDs(std::span<uint64_t> outputNodeIDs) { circuit_object_->outputs.assign(outputNodeIDs.begin(), outputNodeIDs.end()); }

CircuitObjectWrapper::MutableNode CircuitObjectWrapper::getNodeWithID(uint64_t nodeID) {
    for (auto& node : circuit_object_->nodes) {
        if (node->id == nodeID) {
            return NodeObjectWrapper(node.get());
        }
    }
    throw std::runtime_error("Node could not be found with ID: " + nodeID);
}

uint64_t CircuitObjectWrapper::getNextID() const {
    uint64_t currID = 0;
    for (auto& node : circuit_object_->nodes) {
        if (node->id > currID) {
            currID = node->id;
        }
    }
    return currID + 1;
}

CircuitObjectWrapper::MutableNode CircuitObjectWrapper::addNode() {
    auto temp = std::make_unique<core::ir::NodeTableT>();
    temp->id = getNextID();
    circuit_object_->nodes.push_back(std::move(temp));
    return NodeObjectWrapper(circuit_object_->nodes.back().get());
}

CircuitObjectWrapper::MutableNode CircuitObjectWrapper::addNode(long position) {
    if (position < 0) {
        return addNode();
    }
    auto temp = std::make_unique<core::ir::NodeTableT>();
    temp->id = getNextID();
    auto itPos = circuit_object_->nodes.begin() + position;
    auto newIt = circuit_object_->nodes.insert(itPos, std::move(temp));
    return NodeObjectWrapper(newIt->get());
}

/**
 * @brief restores the topological order of the nodes for the circuit in a iterative fashion
 *
 * @param nodeID ID of the node that was just inserted (after all inputs) and that might violate the constraint due to output nodes (before the respective node)
 */
void CircuitObjectWrapper::iterativelyRestoreTopologicalOrder(uint64_t nodeID, std::unordered_map<uint64_t, std::unordered_set<uint64_t>> nodeSuccessors) {
    // iteratively work with working_set
    std::deque<uint64_t> working_set;
    working_set.push_back(nodeID);

    // process each element in the working set
    while (!working_set.empty()) {
        // extract first element from working set
        uint64_t cur_node = *working_set.begin();
        working_set.pop_front();
        std::vector<std::unique_ptr<fuse::core::ir::NodeTableT>> nodes_to_move;
        std::vector<uint64_t> ids_to_move;

        // check if outputs are before current node (violation of constraint)
        // (when topological order is violated)
        auto it = circuit_object_->nodes.begin();
        while (it != circuit_object_->nodes.end()) {
            // current node reached
            if (((*it)->id) == cur_node) {
                // insert output nodes that were found before current node
                // right after current node
                it++;

                for (auto nodeit = nodes_to_move.begin(), end = nodes_to_move.end(); nodeit != end; ++nodeit) {
                    ids_to_move.push_back((*nodeit)->id);
                    circuit_object_->nodes.insert(it, std::move(*nodeit));
                }

                // recursion call: make sure moving did not violate a constraint
                for (auto nodeid = ids_to_move.begin(), end = ids_to_move.end(); nodeid != end; ++nodeid) {
                    working_set.push_back(*nodeid);
                }

                // break;
                // TODO THIS IS JUST FOR TESTING PURPOSES!
                // MAKE SURE TO RESTORE THE BREAK STATEMENT INSTEAD OF RETURN!
                return;
            }

            // output node found before current node
            // move to list (delete entry from nodes)
            if (nodeSuccessors[cur_node].contains((*it)->id)) {
                // move element
                auto node_obj = std::move(*it);
                nodes_to_move.push_back(std::move(node_obj));

                // erase and get next element
                it = circuit_object_->nodes.erase(it);
                continue;
            }
            it++;
        }
    }
}

/*
 * nodesToReplace: all the nodes that are to be deleted for the one call node
 */
uint64_t CircuitObjectWrapper::replaceNodesBySubcircuit(CircuitReadOnly& subcircuit,
                                                    // nodes that need to be deleted afterwards
                                                    std::span<uint64_t> nodesToReplace,
                                                    // subcircuit input to nodes in original circuit that contain the input values
                                                    std::unordered_map<uint64_t, uint64_t> subcircuitInputToCircuitNode,
                                                    // to circuit nodes that use this output as input
                                                    std::unordered_map<uint64_t, std::vector<uint64_t>> subcircuitOutputToCircuitNodes,
                                                    // maps subcircuit output to one of the nodes to replace
                                                    std::unordered_map<uint64_t, uint64_t> subcircuitOutputToReplacedCircuitNode) {
    using Identifier = uint64_t;
    using Offset = uint32_t;
    // call node needs:
    //      subcircuit name
    //      input node IDs
    //      input node offsets
    //      operation set to "call subcircuit"
    auto subCircName = subcircuit.getName();
    std::vector<Identifier> inputNodeIDs;

    for (auto subcircuitInput : subcircuit.getInputNodeIDs()) {
        auto node = getNodeWithID(subcircuitInputToCircuitNode[subcircuitInput]);
        inputNodeIDs.push_back(node.getNodeID());
    }

    // define call node for the first node to replace -> replaced in the correct place
    auto callNodeObj = std::make_unique<core::ir::NodeTableT>();
    Identifier callNodeID = getNextID();
    callNodeObj->id = callNodeID;
    callNodeObj->subcircuit_name = subCircName;
    callNodeObj->operation = core::ir::PrimitiveOperation::CallSubcircuit;
    callNodeObj->input_identifiers = inputNodeIDs;
    callNodeObj->num_of_outputs = subcircuitOutputToReplacedCircuitNode.size();

    std::unordered_set<Identifier> workingSet;
    for (auto pair : subcircuitInputToCircuitNode) {
        workingSet.insert(pair.second);
    }
    for (auto it = circuit_object_->nodes.begin(), end = circuit_object_->nodes.end(); it != end; ++it) {
        if (workingSet.contains((*it)->id)) {
            workingSet.erase((*it)->id);
        }
        if (workingSet.empty()) {
            ++it;
            circuit_object_->nodes.insert(it, std::move(callNodeObj));
            break;
        }
    }

    // for all ongoing nodes that reference the replaced node:
    //      set input node ID to the call node
    //      add input offset to the correct output of the call node
    std::unordered_map<Identifier, Offset>
        subcircOutputToCallOffset;
    uint32_t offset = 0;
    for (auto subcircuitOutput : subcircuit.getOutputNodeIDs()) {
        subcircOutputToCallOffset[subcircuitOutput] = offset++;
    }

    // for all subcircuit outputs : update dependencies on these output values
    for (auto subcircuitOutput : subcircuit.getOutputNodeIDs()) {
        Identifier inputToBeReplaced = subcircuitOutputToReplacedCircuitNode[subcircuitOutput];  // 33
        // update all nodes that shall now take the subcircuit as input
        for (auto nodeWithInputToBeReplacedID : subcircuitOutputToCircuitNodes[subcircuitOutput]) {  // 47
            auto node = getNodeWithID(nodeWithInputToBeReplacedID);
            // find out which input + offset need to be updated and set them to call node ID + output offset
            for (size_t input = 0; input < node.getNumberOfInputs(); ++input) {
                auto inNodeID = node.getInputNodeIDs()[input];
                if (inNodeID == inputToBeReplaced) {
                    // update both input node ID + input node offset
                    if (node.usesInputOffsets()) {
                        auto inOffset = node.getInputOffsets()[input];
                        node.replaceInputBy(inNodeID, callNodeID, inOffset, subcircOutputToCallOffset[subcircuitOutput]);
                    }
                    // update input node ID and introduce input offset if necessary
                    else {
                        node.replaceInputBy(inNodeID, callNodeID, 0, subcircOutputToCallOffset[subcircuitOutput]);
                    }
                }
            }
        }
    }

    // delete all nodes that have been replaced by the call
    std::unordered_set<uint64_t> nodesToDel;
    for (auto nodeID : nodesToReplace) {
        nodesToDel.insert(nodeID);
    }
    removeNodes(nodesToDel);

    // restore topological order of nodes
    std::unordered_map<uint64_t, std::unordered_set<uint64_t>> nodeSuccessors = fuse::passes::getNodeSuccessors(*this);
    iterativelyRestoreTopologicalOrder(callNodeID, nodeSuccessors);

    return callNodeID;
}

void CircuitObjectWrapper::replaceNodesBySIMDNode(std::span<uint64_t> nodesToSimdify) {
    using Identifier = uint64_t;
    using Offset = uint32_t;
    // gather input nodes and offsets for the SIMD node
    // and map each node to one output by mapping the previous node to the newly produced output offset
    std::vector<Identifier> inputNodeIDs;
    std::vector<Offset> inputNodeOffsets;
    std::unordered_map<Identifier, Offset> prevNodeToOffset;

    Offset currOffset = 0;
    for (size_t it = 0; it < nodesToSimdify.size(); ++it) {
        auto node = getNodeWithID(nodesToSimdify[it]);
        inputNodeIDs.insert(inputNodeIDs.end(), node.getInputNodeIDs().begin(), node.getInputNodeIDs().end());
        if (node.usesInputOffsets()) {
            inputNodeOffsets.insert(inputNodeOffsets.end(), node.getInputOffsets().begin(), node.getInputOffsets().end());
        } else {
            inputNodeOffsets.insert(inputNodeOffsets.end(), node.getNumberOfInputs(), 0);
        }
        // node output is now the current offset inside the SIMD node
        prevNodeToOffset[node.getNodeID()] = currOffset++;
    }

    // each two inputs produce one output
    // assert((inputNodeIDs.size() / 2) == (currOffset + 1));
    auto simdNodeObj = std::make_unique<core::ir::NodeTableT>();
    Identifier simdNodeID = getNextID();
    simdNodeObj->input_identifiers = inputNodeIDs;
    simdNodeObj->input_offsets = inputNodeOffsets;
    simdNodeObj->operation = getNodeWithID(nodesToSimdify[0]).getOperation();
    if (getNodeWithID(nodesToSimdify[0]).isBinaryNode()) {
        simdNodeObj->num_of_outputs = inputNodeIDs.size() / 2;
    } else if (simdNodeObj->operation == fuse::core::ir::PrimitiveOperation::Mux) {
        simdNodeObj->num_of_outputs = inputNodeIDs.size() / 3;
    } else {  // Unary
        simdNodeObj->num_of_outputs = inputNodeIDs.size();
    }
    simdNodeObj->id = simdNodeID;

    std::unordered_set<Identifier> workingSet;
    for (auto inputID : inputNodeIDs) {
        workingSet.insert(inputID);
    }
    for (auto it = circuit_object_->nodes.begin(), end = circuit_object_->nodes.end(); it != end; ++it) {
        if (workingSet.contains((*it)->id)) {
            workingSet.erase((*it)->id);
        }
        if (workingSet.empty()) {
            ++it;
            circuit_object_->nodes.insert(it, std::move(simdNodeObj));
            break;
        }
    }

    // for all nodes that referred to a simdified node: set input ID + Offset
    for (auto node : *this) {
        for (size_t input = 0; input < node.getNumberOfInputs(); ++input) {
            auto inNode = node.getInputNodeIDs()[input];
            if (prevNodeToOffset.contains(inNode)) {
                if (node.usesInputOffsets()) {
                    auto inOffset = node.getInputOffsets()[input];
                    node.replaceInputBy(inNode, simdNodeID, inOffset, prevNodeToOffset[inNode]);
                } else {
                    node.replaceInputBy(inNode, simdNodeID, 0, prevNodeToOffset[inNode]);
                }
            }
        }
    }

    // remove the remaining nodes from the circuit
    std::unordered_set<uint64_t> nodesToDel;
    for (auto nodeID : nodesToSimdify) {
        nodesToDel.insert(nodeID);
    }
    removeNodes(nodesToDel);

    // restore topological order of nodes
    std::unordered_map<uint64_t, std::unordered_set<uint64_t>> nodeSuccessors = fuse::passes::getNodeSuccessors(*this);
    iterativelyRestoreTopologicalOrder(simdNodeID, nodeSuccessors);
}

void CircuitObjectWrapper::removeNode(uint64_t nodeToDelete) {
    std::erase_if(circuit_object_->nodes, [=](std::unique_ptr<fuse::core::ir::NodeTableT>& node) { return node->id == nodeToDelete; });
}

void CircuitObjectWrapper::removeNodes(const std::unordered_set<uint64_t>& nodesToDelete) {
    std::erase_if(circuit_object_->nodes, [=](std::unique_ptr<fuse::core::ir::NodeTableT>& node) { return nodesToDelete.contains(node->id); });
}

void CircuitObjectWrapper::removeNodesNotContainedIn(const std::unordered_set<uint64_t>& nodesToKeep) {
    std::erase_if(circuit_object_->nodes, [=](std::unique_ptr<fuse::core::ir::NodeTableT>& node) { return !nodesToKeep.contains(node->id); });
}

/*
 ****************************************** ModuleBufferWrapper Member Functions ******************************************
 */
std::string ModuleBufferWrapper::getModuleAnnotations() const {
    return module_flatbuffer_->module_annotations()->str();
}

std::string ModuleBufferWrapper::getStringValueForAttribute(std::string attribute) const {
    std::regex attribute_regex(attribute + "\\s*:\\s*(\\d*)\\s*,?");
    std::smatch match;
    auto annotation_string = getModuleAnnotations();
    std::regex_search(annotation_string, match, attribute_regex);
    return match.str();
}

std::string ModuleBufferWrapper::getEntryCircuitName() const {
    return module_flatbuffer_->entry_point()->str();
}

ModuleBufferWrapper::Circuit ModuleBufferWrapper::getCircuitWithName(const std::string& name) const {
    auto circs = module_flatbuffer_->circuits();
    for (auto it = circs->begin(), end = circs->end(); it != end; ++it) {
        if (it->circuit_buffer_nested_root()->name()->str() == name) {
            return std::make_unique<CircuitBufferWrapper>(it->circuit_buffer_nested_root());
        }
    }
    throw std::logic_error("Module does not contain a circuit with the name: " + name);
}

ModuleBufferWrapper::Circuit ModuleBufferWrapper::getEntryCircuit() const {
    return getCircuitWithName(module_flatbuffer_->entry_point()->str());
}

std::vector<std::string> ModuleBufferWrapper::getAllCircuitNames() const {
    std::vector<std::string> result;
    auto circs = module_flatbuffer_->circuits();
    for (auto it = circs->begin(), end = circs->end(); it != end; ++it) {
        result.push_back(it->circuit_buffer_nested_root()->name()->str());
    }
    return result;
}

/*
 ****************************************** ModuleObjectWrapper Member Functions ******************************************
 */
std::string ModuleObjectWrapper::getEntryCircuitName() const {
    return module_object_->entry_point;
}

std::string ModuleObjectWrapper::getModuleAnnotations() const {
    return module_object_->module_annotations;
}

std::string ModuleObjectWrapper::getStringValueForAttribute(std::string attribute) const {
    std::regex attribute_regex(attribute + "\\s*:\\s*(\\w*)\\s*,?");
    std::smatch match;
    auto annotation_string = getModuleAnnotations();
    std::regex_search(annotation_string, match, attribute_regex);
    return match.str();
}

ModuleObjectWrapper::Circuit ModuleObjectWrapper::getCircuitWithName(const std::string& name) const {
    for (auto& it : unpacked_circuits_) {
        if (it->name == name) {
            return std::make_unique<CircuitObjectWrapper>(it.get());
        }
    }
    for (auto& it : module_object_->circuits) {
        const ir::CircuitTable* circuitBuffer = ir::GetCircuitTable(it->circuit_buffer.data());
        if (circuitBuffer->name()->str() == name) {
            return std::make_unique<CircuitBufferWrapper>(circuitBuffer);
        }
    }
    throw std::logic_error("Module does not contain a circuit with the name: " + name);
}

ModuleObjectWrapper::Circuit ModuleObjectWrapper::getEntryCircuit() const {
    return getCircuitWithName(module_object_->entry_point);
}

void ModuleObjectWrapper::setEntryCircuitName(const std::string& name) {
    module_object_->entry_point = name;
}

void ModuleObjectWrapper::setModuleAnnotations(const std::string& annotations) {
    module_object_->module_annotations = annotations;
}

void ModuleObjectWrapper::setStringValueForAttribute(const std::string& attribute, const std::string& value) {
    std::regex attribute_regex(attribute + "\\s*:\\s*(\\w*)\\s*,?");
    std::regex_replace(module_object_->module_annotations, attribute_regex, value);
}

ModuleObjectWrapper::MutableCircuit ModuleObjectWrapper::getCircuitWithName(const std::string& name) {
    // Look if the circuit needs to be unpacked first
    for (auto& it : module_object_->circuits) {
        const ir::CircuitTable* circuitBuffer = ir::GetCircuitTable(it->circuit_buffer.data());
        if (circuitBuffer->name()->str() == name) {
            // unpack and save circuit first
            std::unique_ptr<ir::CircuitTableT> ptr;
            // this little trick is necessary because it's impossible to directly save the unpacked data using std::make_unique<>
            ptr.reset(circuitBuffer->UnPack());
            unpacked_circuits_.push_back(std::move(ptr));
            // then remove the old flatbuffer data from the module, as the circuit has been unpacked anyways
            std::erase_if(module_object_->circuits, [&, name](auto const& buffer) { return ir::GetCircuitTable(buffer->circuit_buffer.data())->name()->str() == name; });
        }
    }
    // Then return a wrapper to the unpacked circuit
    for (auto& it : unpacked_circuits_) {
        if (it->name == name) {
            return CircuitObjectWrapper(it.get());
        }
    }
    throw std::logic_error("Module does not contain a circuit with the name: " + name);
}

ModuleObjectWrapper::MutableCircuit ModuleObjectWrapper::getEntryCircuit() {
    return getCircuitWithName(module_object_->entry_point);
}

std::vector<std::string> ModuleObjectWrapper::getAllCircuitNames() const {
    std::vector<std::string> result;
    // first write all of the unpacked circuits' names
    for (auto& circ : unpacked_circuits_) {
        result.push_back(circ->name);
    }
    // then iterate through all the binaries and get the names through the flatbuffers accessors
    for (auto& it : module_object_->circuits) {
        const ir::CircuitTable* circuitBuffer = ir::GetCircuitTable(it->circuit_buffer.data());
        result.push_back(circuitBuffer->name()->str());
    }
    return result;
}

void ModuleObjectWrapper::removeCircuit(const std::string& name) {
    // either erase from the vector that contains the flatbuffer binaries
    std::erase_if(module_object_->circuits, [&, name](auto const& buffer) { return ir::GetCircuitTable(buffer->circuit_buffer.data())->name()->str() == name; });
    // or erase from the vector that contains the unpacked circuits
    std::erase_if(unpacked_circuits_, [&, name](auto const& unpacked) { return unpacked->name == name; });
}

}  // namespace fuse::core
