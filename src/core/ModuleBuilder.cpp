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

#include "ModuleBuilder.h"

#include <deque>
#include <iostream>
#include <queue>
#include <stdexcept>

namespace fuse::frontend {

/*
 * Circuit Builder
 */

void CircuitBuilder::addNode(Identifier id,
                             const std::vector<size_t> &input_datatypes,
                             const std::vector<Identifier> &input_identifiers,
                             const std::vector<unsigned int> &input_offsets,
                             ir::PrimitiveOperation operation,
                             const std::string &custom_operation_name,
                             const std::string &subcircuit_name,
                             const std::vector<unsigned char> &payload,
                             unsigned int num_of_outputs,
                             const std::vector<size_t> &output_datatypes,
                             const std::string &node_annotations) {
    bool serializeInputDatatypes = !input_datatypes.empty();
    bool serializeInputIdentifiers = !input_identifiers.empty();
    bool serializeInputOffsets = !input_offsets.empty();
    bool serializeCustomOperationName = !custom_operation_name.empty();
    bool serializeSubCircuitName = !subcircuit_name.empty();
    bool serializePayload = !payload.empty();
    bool serializeOutputDatatypes = !output_datatypes.empty();
    bool serializeNodeAnnotations = !node_annotations.empty();

    // serialize all strings
    flatbuffers::Offset<flatbuffers::String> customOperationNameString;
    if (serializeCustomOperationName) {
        customOperationNameString = circuitBuilder_.CreateString(custom_operation_name);
    }

    flatbuffers::Offset<flatbuffers::String> subCircuitNameString;
    if (serializeSubCircuitName) {
        subCircuitNameString = circuitBuilder_.CreateString(subcircuit_name);
    }

    flatbuffers::Offset<flatbuffers::String> nodeAnnotationString;
    if (serializeNodeAnnotations) {
        nodeAnnotationString = circuitBuilder_.CreateString(node_annotations);
    }

    // serialize input data types
    std::vector<flatbuffers::Offset<ir::DataTypeTable>> inputTypes;
    for (auto inputIndex : input_datatypes) {
        inputTypes.push_back(dataTypes_.at(inputIndex));
    }
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<ir::DataTypeTable>>> inputTypesVector;
    if (serializeInputDatatypes) {
        inputTypesVector = circuitBuilder_.CreateVector(inputTypes);
    }

    // serialize output data types
    std::vector<flatbuffers::Offset<ir::DataTypeTable>> outputTypes;
    for (auto outputIndex : output_datatypes) {
        outputTypes.push_back(dataTypes_.at(outputIndex));
    }
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<ir::DataTypeTable>>> outputTypesVector;
    if (serializeOutputDatatypes) {
        outputTypesVector = circuitBuilder_.CreateVector(outputTypes);
    }

    // serialize input identifiers vector
    flatbuffers::Offset<flatbuffers::Vector<Identifier>> inputIdentifiersVector;
    if (serializeInputIdentifiers) {
        inputIdentifiersVector = circuitBuilder_.CreateVector(input_identifiers);
    }

    // serialize input offset vector
    flatbuffers::Offset<flatbuffers::Vector<unsigned int>> inputOffsetVector;
    if (serializeInputOffsets) {
        inputOffsetVector = circuitBuilder_.CreateVector(input_offsets);
    }

    // serialize payload vector
    flatbuffers::Offset<flatbuffers::Vector<unsigned char>> payloadVector;
    if (serializePayload) {
        payloadVector = circuitBuilder_.CreateVector(payload);
    }

    // read out node id and check if it was a custom one:
    // every ID < nextID_-1 cannot be assigned and any ID in customIDs_ also not
    assert(!(customIDs_.contains(id)) || !(id + 1 < nextID_));
    if (id >= nextID_) {
        customIDs_.insert(id);
    }

    // serialize whole node with the previously serialized data
    ir::NodeTableBuilder nodeBuilder(circuitBuilder_);
    nodeBuilder.add_id(id);
    if (serializeInputDatatypes) {
        nodeBuilder.add_input_datatypes(inputTypesVector);
    }
    if (serializeInputIdentifiers) {
        nodeBuilder.add_input_identifiers(inputIdentifiersVector);
    }
    if (serializeInputOffsets) {
        nodeBuilder.add_input_offsets(inputOffsetVector);
    }
    nodeBuilder.add_operation(operation);
    if (serializeCustomOperationName) {
        nodeBuilder.add_custom_op_name(customOperationNameString);
    }
    if (serializeSubCircuitName) {
        nodeBuilder.add_subcircuit_name(subCircuitNameString);
    }
    nodeBuilder.add_num_of_outputs(num_of_outputs);
    if (serializeOutputDatatypes) {
        nodeBuilder.add_output_datatypes(outputTypesVector);
    }
    if (serializePayload) {
        nodeBuilder.add_payload(payloadVector);
    }
    if (serializeNodeAnnotations) {
        nodeBuilder.add_node_annotations(nodeAnnotationString);
    }
    auto nodeOffset = nodeBuilder.Finish();

    // save node offset for circuit
    nodes_.push_back(nodeOffset);
}

Identifier CircuitBuilder::addNode(const std::vector<size_t> &input_datatypes,
                                   const std::vector<Identifier> &input_identifiers,
                                   const std::vector<unsigned int> &input_offsets, ir::PrimitiveOperation operation,
                                   const std::string &custom_operation_name, const std::string &subcircuit_name,
                                   const std::vector<unsigned char> &payload,
                                   unsigned int num_of_outputs, const std::vector<size_t> &output_datatypes,
                                   const std::string &node_annotations) {
    // read out node id and increment afterwards
    unsigned long nodeID = getNextID();
    addNode(nodeID, input_datatypes, input_identifiers, input_offsets, operation, custom_operation_name, subcircuit_name, payload, num_of_outputs, output_datatypes, node_annotations);
    return nodeID;
}

void CircuitBuilder::addAnnotations(const std::string &annotations) {
    annotations_ += annotations;
}

size_t CircuitBuilder::addDataType(ir::PrimitiveType primitiveType,
                                   ir::SecurityLevel securityLevel,
                                   const std::vector<int64_t> &shape,
                                   const std::string &data_type_annotations) {
    bool serializeShape = !shape.empty();
    bool serializeDataTypeAnnotations = !data_type_annotations.empty();

    // serialize annotation string
    flatbuffers::Offset<flatbuffers::String> annotationString;
    if (serializeDataTypeAnnotations) {
        annotationString = circuitBuilder_.CreateString(data_type_annotations);
    }

    // serialize shape vector
    flatbuffers::Offset<flatbuffers::Vector<int64_t>> shapeVector;
    if (serializeShape) {
        shapeVector = circuitBuilder_.CreateVector(shape);
    }

    ir::DataTypeTableBuilder dataTypeBuilder(circuitBuilder_);
    dataTypeBuilder.add_primitive_type(primitiveType);
    if (serializeDataTypeAnnotations) {
        dataTypeBuilder.add_data_type_annotations(annotationString);
    }
    dataTypeBuilder.add_security_level(securityLevel);
    if (serializeShape) {
        dataTypeBuilder.add_shape(shapeVector);
    }
    auto dataTypeOffset = dataTypeBuilder.Finish();

    dataTypes_.push_back(dataTypeOffset);

    // last index of vector is the index of the newly constructed datatype
    return dataTypes_.size() - 1;
}

Identifier CircuitBuilder::addInputNode(size_t inputType, const std::string &nodeAnnotations) {
    inputDataTypes_.push_back(inputType);
    auto inputID = addNode({inputType}, {}, {}, ir::PrimitiveOperation::Input, "", "", {}, 1, {inputType}, nodeAnnotations);
    inputIdentifiers_.push_back(inputID);
    return inputID;
}

void CircuitBuilder::addInputNode(Identifier nodeID, size_t inputType, const std::string &nodeAnnotations) {
    inputDataTypes_.push_back(inputType);
    addNode(nodeID, {inputType}, {}, {}, ir::PrimitiveOperation::Input, "", "", {}, 1, {inputType}, nodeAnnotations);
    inputIdentifiers_.push_back(nodeID);
}

Identifier CircuitBuilder::addOutputNode(size_t outputType, const std::vector<Identifier> &inputNodeIdentifiers, const std::vector<unsigned int> &input_offsets) {
    outputDataTypes_.push_back(outputType);
    auto outputID = addNode({outputType}, inputNodeIdentifiers, input_offsets, ir::PrimitiveOperation::Output, "", "", {}, 1, {outputType},
                            "");
    outputIdentifiers_.push_back(outputID);
    return outputID;
}

void CircuitBuilder::addOutputNode(Identifier nodeID, size_t outputType, const std::vector<Identifier> &inputNodeIdentifiers, const std::vector<unsigned int> &input_offsets) {
    outputDataTypes_.push_back(outputType);
    addNode(nodeID, {outputType}, inputNodeIdentifiers, input_offsets, ir::PrimitiveOperation::Output, "", "", {}, 1, {outputType},
            "");
    outputIdentifiers_.push_back(nodeID);
}

Identifier CircuitBuilder::addInputNode(std::vector<size_t> inputTypes, const std::string &nodeAnnotations) {
    inputDataTypes_.insert(inputDataTypes_.end(), inputTypes.begin(), inputTypes.end());
    auto inputID = addNode(inputTypes, {}, {}, ir::PrimitiveOperation::Input, "", "", {}, inputTypes.size(), inputTypes, nodeAnnotations);
    inputIdentifiers_.push_back(inputID);
    return inputID;
}

void CircuitBuilder::addInputNode(Identifier nodeID, std::vector<size_t> inputTypes, const std::string &nodeAnnotations) {
    inputDataTypes_.insert(inputDataTypes_.end(), inputTypes.begin(), inputTypes.end());
    addNode(nodeID, inputTypes, {}, {}, ir::PrimitiveOperation::Input, "", "", {}, inputTypes.size(), inputTypes, nodeAnnotations);
    inputIdentifiers_.push_back(nodeID);
}

Identifier CircuitBuilder::addOutputNode(std::vector<size_t> outputTypes, const std::vector<Identifier> &inputNodeIdentifiers, const std::vector<unsigned int> &input_offsets) {
    outputDataTypes_.insert(outputDataTypes_.end(), outputTypes.begin(), outputTypes.end());
    auto outputID = addNode(outputTypes, inputNodeIdentifiers, input_offsets, ir::PrimitiveOperation::Output, "", "", {}, outputTypes.size(), outputTypes,
                            "");
    outputIdentifiers_.push_back(outputID);
    return outputID;
}

void CircuitBuilder::addOutputNode(Identifier nodeID, std::vector<size_t> outputTypes, const std::vector<Identifier> &inputNodeIdentifiers, const std::vector<unsigned int> &input_offsets) {
    outputDataTypes_.insert(outputDataTypes_.end(), outputTypes.begin(), outputTypes.end());
    addNode(nodeID, outputTypes, inputNodeIdentifiers, input_offsets, ir::PrimitiveOperation::Output, "", "", {}, outputTypes.size(), outputTypes,
            "");
    outputIdentifiers_.push_back(nodeID);
}

Identifier
CircuitBuilder::addNode(ir::PrimitiveOperation operation, const std::vector<Identifier> &inputNodeIdentifiers, const std::vector<unsigned int> &inputOffsets, const std::string &nodeAnnotations) {
    return addNode({}, inputNodeIdentifiers, inputOffsets, operation, "", "", {}, 1, {}, nodeAnnotations);
}
Identifier CircuitBuilder::addNodeWithNumberOfOutputs(ir::PrimitiveOperation operation,
                                                      const std::vector<Identifier> &inputNodeIdentifiers,
                                                      const std::vector<unsigned int> &inputOffsets,
                                                      unsigned int numberOfOutputs,
                                                      const std::string &nodeAnnotations) {
    return addNode({}, inputNodeIdentifiers, inputOffsets, operation, "", "", {}, numberOfOutputs, {}, nodeAnnotations);
}

void CircuitBuilder::addNode(Identifier nodeID, ir::PrimitiveOperation operation, const std::vector<Identifier> &inputNodeIdentifiers, const std::vector<unsigned int> &inputOffsets) {
    addNode(nodeID, {}, inputNodeIdentifiers, inputOffsets, operation, "", "", {}, 1, {}, "");
}

Identifier CircuitBuilder::addConstantNodeWithPayload(bool payload) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Bool, ir::SecurityLevel::Plaintext);
    flexbuffers::Builder fbb;
    fbb.Bool(payload);
    fbb.Finish();
    return addNode({}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}

void CircuitBuilder::addConstantNodeWithPayload(Identifier nodeID, bool payload) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Bool, ir::SecurityLevel::Plaintext);
    flexbuffers::Builder fbb;
    fbb.Bool(payload);
    fbb.Finish();
    addNode(nodeID, {}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}

Identifier CircuitBuilder::addConstantNodeWithPayload(int8_t payload) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Int8, ir::SecurityLevel::Plaintext);
    flexbuffers::Builder fbb;
    fbb.Int(payload);
    fbb.Finish();
    return addNode({}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}
void CircuitBuilder::addConstantNodeWithPayload(Identifier nodeID, int8_t payload) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Int8, ir::SecurityLevel::Plaintext);
    flexbuffers::Builder fbb;
    fbb.Int(payload);
    fbb.Finish();
    addNode(nodeID, {}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}
Identifier CircuitBuilder::addConstantNodeWithPayload(int16_t payload) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Int16, ir::SecurityLevel::Plaintext);
    flexbuffers::Builder fbb;
    fbb.Int(payload);
    fbb.Finish();
    return addNode({}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}
void CircuitBuilder::addConstantNodeWithPayload(Identifier nodeID, int16_t payload) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Int16, ir::SecurityLevel::Plaintext);
    flexbuffers::Builder fbb;
    fbb.Int(payload);
    fbb.Finish();
    addNode(nodeID, {}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}
Identifier CircuitBuilder::addConstantNodeWithPayload(int32_t payload) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Int32, ir::SecurityLevel::Plaintext);
    flexbuffers::Builder fbb;
    fbb.Int(payload);
    fbb.Finish();
    return addNode({}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}
void CircuitBuilder::addConstantNodeWithPayload(Identifier nodeID, int32_t payload) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Int32, ir::SecurityLevel::Plaintext);
    flexbuffers::Builder fbb;
    fbb.Int(payload);
    fbb.Finish();
    addNode(nodeID, {}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}
Identifier CircuitBuilder::addConstantNodeWithPayload(int64_t payload) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Int64, ir::SecurityLevel::Plaintext);
    flexbuffers::Builder fbb;
    fbb.Int(payload);
    fbb.Finish();
    return addNode({}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}
void CircuitBuilder::addConstantNodeWithPayload(Identifier nodeID, int64_t payload) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Int64, ir::SecurityLevel::Plaintext);
    flexbuffers::Builder fbb;
    fbb.Int(payload);
    fbb.Finish();
    addNode(nodeID, {}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}
Identifier CircuitBuilder::addConstantNodeWithPayload(uint8_t payload) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::UInt8, ir::SecurityLevel::Plaintext);
    flexbuffers::Builder fbb;
    fbb.UInt(payload);
    fbb.Finish();
    return addNode({}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}
void CircuitBuilder::addConstantNodeWithPayload(Identifier nodeID, uint8_t payload) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::UInt8, ir::SecurityLevel::Plaintext);
    flexbuffers::Builder fbb;
    fbb.UInt(payload);
    fbb.Finish();
    addNode(nodeID, {}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}
Identifier CircuitBuilder::addConstantNodeWithPayload(uint16_t payload) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::UInt16, ir::SecurityLevel::Plaintext);
    flexbuffers::Builder fbb;
    fbb.UInt(payload);
    fbb.Finish();
    return addNode({}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}
void CircuitBuilder::addConstantNodeWithPayload(Identifier nodeID, uint16_t payload) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::UInt16, ir::SecurityLevel::Plaintext);
    flexbuffers::Builder fbb;
    fbb.UInt(payload);
    fbb.Finish();
    addNode(nodeID, {}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}
Identifier CircuitBuilder::addConstantNodeWithPayload(uint32_t payload) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::UInt32, ir::SecurityLevel::Plaintext);
    flexbuffers::Builder fbb;
    fbb.UInt(payload);
    fbb.Finish();
    return addNode({}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}
void CircuitBuilder::addConstantNodeWithPayload(Identifier nodeID, uint32_t payload) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::UInt32, ir::SecurityLevel::Plaintext);
    flexbuffers::Builder fbb;
    fbb.UInt(payload);
    fbb.Finish();
    addNode(nodeID, {}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}
Identifier CircuitBuilder::addConstantNodeWithPayload(uint64_t payload) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::UInt64, ir::SecurityLevel::Plaintext);
    flexbuffers::Builder fbb;
    fbb.UInt(payload);
    fbb.Finish();
    return addNode({}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}
void CircuitBuilder::addConstantNodeWithPayload(Identifier nodeID, uint64_t payload) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::UInt64, ir::SecurityLevel::Plaintext);
    flexbuffers::Builder fbb;
    fbb.UInt(payload);
    fbb.Finish();
    addNode(nodeID, {}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}

Identifier CircuitBuilder::addConstantNodeWithPayload(float payload) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Float, ir::SecurityLevel::Plaintext);
    flexbuffers::Builder fbb;
    fbb.Float(payload);
    fbb.Finish();
    return addNode({}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}
void CircuitBuilder::addConstantNodeWithPayload(Identifier nodeID, float payload) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Float, ir::SecurityLevel::Plaintext);
    flexbuffers::Builder fbb;
    fbb.Float(payload);
    fbb.Finish();
    addNode(nodeID, {}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}
Identifier CircuitBuilder::addConstantNodeWithPayload(double payload) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Double, ir::SecurityLevel::Plaintext);
    flexbuffers::Builder fbb;
    fbb.Double(payload);
    fbb.Finish();
    return addNode({}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}
void CircuitBuilder::addConstantNodeWithPayload(Identifier nodeID, double payload) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Double, ir::SecurityLevel::Plaintext);
    flexbuffers::Builder fbb;
    fbb.Double(payload);
    fbb.Finish();
    addNode(nodeID, {}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}

Identifier CircuitBuilder::addConstantNodeWithBinaryPayload(const std::vector<unsigned char> &blobPayload, const std::string &annotations) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::UInt8, ir::SecurityLevel::Plaintext, {static_cast<long>(blobPayload.size())});
    flexbuffers::Builder fbb;
    fbb.Blob(blobPayload);
    fbb.Finish();
    return addNode({}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, annotations);
}
void CircuitBuilder::addConstantNodeWithBinaryPayload(Identifier nodeID, const std::vector<unsigned char> &blobPayload) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::UInt8, ir::SecurityLevel::Plaintext, {static_cast<long>(blobPayload.size())});
    flexbuffers::Builder fbb;
    fbb.Blob(blobPayload);
    fbb.Finish();
    addNode(nodeID, {}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}

Identifier CircuitBuilder::addConstantNodeWithPayload(const std::vector<bool> &boolVector, const std::string &annotations) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Bool, ir::SecurityLevel::Plaintext, {static_cast<long>(boolVector.size())});
    flexbuffers::Builder fbb;
    size_t startVec = fbb.StartVector();
    for (bool val : boolVector) {
        fbb.Bool(val);
    }
    fbb.EndVector(startVec, true, true);
    fbb.Finish();
    return addNode({}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, annotations);
}

void CircuitBuilder::addConstantNodeWithPayload(Identifier nodeID, const std::vector<bool> &boolVector) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Bool, ir::SecurityLevel::Plaintext, {static_cast<long>(boolVector.size())});
    flexbuffers::Builder fbb;
    size_t startVec = fbb.StartVector();
    for (bool val : boolVector) {
        fbb.Bool(val);
    }
    fbb.EndVector(startVec, true, true);
    fbb.Finish();
    addNode(nodeID, {}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}

Identifier CircuitBuilder::addConstantNodeWithPayload(const std::vector<std::vector<bool>> &boolMatrix, const std::string &annotations) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Bool, ir::SecurityLevel::Plaintext, {static_cast<long>(boolMatrix.size()), static_cast<long>(boolMatrix.at(0).size())});
    flexbuffers::Builder fbb;
    size_t startMat = fbb.StartVector();
    for (auto &vec : boolMatrix) {
        size_t startVec = fbb.StartVector();
        for (bool val : vec) {
            fbb.Bool(val);
        }
        fbb.EndVector(startVec, true, true);
    }
    fbb.EndVector(startMat, true, true);
    fbb.Finish();
    return addNode({}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, annotations);
}

Identifier CircuitBuilder::addConstantNodeWithPayload(const std::vector<int8_t> &int8Vector, const std::string &annotations) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Int8, ir::SecurityLevel::Plaintext, {static_cast<long>(int8Vector.size())});
    flexbuffers::Builder fbb;
    fbb.FixedTypedVector(int8Vector.data(), int8Vector.size());
    fbb.Finish();
    return addNode({}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, annotations);
}
void CircuitBuilder::addConstantNodeWithPayload(Identifier nodeID, const std::vector<int8_t> &int8Vector) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Int8, ir::SecurityLevel::Plaintext, {static_cast<long>(int8Vector.size())});
    flexbuffers::Builder fbb;
    fbb.FixedTypedVector(int8Vector.data(), int8Vector.size());
    fbb.Finish();
    addNode(nodeID, {}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}
Identifier CircuitBuilder::addConstantNodeWithPayload(const std::vector<int16_t> &int16Vector, const std::string &annotations) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Int16, ir::SecurityLevel::Plaintext, {static_cast<long>(int16Vector.size())});
    flexbuffers::Builder fbb;
    fbb.FixedTypedVector(int16Vector.data(), int16Vector.size());
    fbb.Finish();
    return addNode({}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, annotations);
}
void CircuitBuilder::addConstantNodeWithPayload(Identifier nodeID, const std::vector<int16_t> &int16Vector) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Int16, ir::SecurityLevel::Plaintext, {static_cast<long>(int16Vector.size())});
    flexbuffers::Builder fbb;
    fbb.FixedTypedVector(int16Vector.data(), int16Vector.size());
    fbb.Finish();
    addNode(nodeID, {}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}
Identifier CircuitBuilder::addConstantNodeWithPayload(const std::vector<int32_t> &int32Vector, const std::string &annotations) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Int32, ir::SecurityLevel::Plaintext, {static_cast<long>(int32Vector.size())});
    flexbuffers::Builder fbb;
    fbb.FixedTypedVector(int32Vector.data(), int32Vector.size());
    fbb.Finish();
    return addNode({}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, annotations);
}
void CircuitBuilder::addConstantNodeWithPayload(Identifier nodeID, const std::vector<int32_t> &int32Vector) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Int32, ir::SecurityLevel::Plaintext, {static_cast<long>(int32Vector.size())});
    flexbuffers::Builder fbb;
    fbb.FixedTypedVector(int32Vector.data(), int32Vector.size());
    fbb.Finish();
    addNode(nodeID, {}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}
Identifier CircuitBuilder::addConstantNodeWithPayload(const std::vector<int64_t> &int64Vector, const std::string &annotations) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Int64, ir::SecurityLevel::Plaintext, {static_cast<long>(int64Vector.size())});
    flexbuffers::Builder fbb;
    fbb.FixedTypedVector(int64Vector.data(), int64Vector.size());
    fbb.Finish();
    return addNode({}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, annotations);
}
void CircuitBuilder::addConstantNodeWithPayload(Identifier nodeID, const std::vector<int64_t> &int64Vector) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Int64, ir::SecurityLevel::Plaintext, {static_cast<long>(int64Vector.size())});
    flexbuffers::Builder fbb;
    fbb.FixedTypedVector(int64Vector.data(), int64Vector.size());
    fbb.Finish();
    addNode(nodeID, {}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}
Identifier CircuitBuilder::addConstantNodeWithPayload(const std::vector<uint8_t> &uint8Vector, const std::string &annotations) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::UInt8, ir::SecurityLevel::Plaintext, {static_cast<long>(uint8Vector.size())});
    flexbuffers::Builder fbb;
    fbb.FixedTypedVector(uint8Vector.data(), uint8Vector.size());
    fbb.Finish();
    return addNode({}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, annotations);
}
void CircuitBuilder::addConstantNodeWithPayload(Identifier nodeID, const std::vector<uint8_t> &uint8Vector) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::UInt8, ir::SecurityLevel::Plaintext, {static_cast<long>(uint8Vector.size())});
    flexbuffers::Builder fbb;
    fbb.FixedTypedVector(uint8Vector.data(), uint8Vector.size());
    fbb.Finish();
    addNode(nodeID, {}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}

Identifier CircuitBuilder::addConstantNodeWithPayload(const std::vector<uint16_t> &uint16Vector, const std::string &annotations) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::UInt16, ir::SecurityLevel::Plaintext, {static_cast<long>(uint16Vector.size())});
    flexbuffers::Builder fbb;
    fbb.FixedTypedVector(uint16Vector.data(), uint16Vector.size());
    fbb.Finish();
    return addNode({}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, annotations);
}
void CircuitBuilder::addConstantNodeWithPayload(Identifier nodeID, const std::vector<uint16_t> &uint16Vector) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::UInt16, ir::SecurityLevel::Plaintext, {static_cast<long>(uint16Vector.size())});
    flexbuffers::Builder fbb;
    fbb.FixedTypedVector(uint16Vector.data(), uint16Vector.size());
    fbb.Finish();
    addNode(nodeID, {}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}
Identifier CircuitBuilder::addConstantNodeWithPayload(const std::vector<uint32_t> &uint32Vector, const std::string &annotations) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::UInt32, ir::SecurityLevel::Plaintext, {static_cast<long>(uint32Vector.size())});
    flexbuffers::Builder fbb;
    fbb.FixedTypedVector(uint32Vector.data(), uint32Vector.size());
    fbb.Finish();
    return addNode({}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, annotations);
}
void CircuitBuilder::addConstantNodeWithPayload(Identifier nodeID, const std::vector<uint32_t> &uint32Vector) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::UInt32, ir::SecurityLevel::Plaintext, {static_cast<long>(uint32Vector.size())});
    flexbuffers::Builder fbb;
    fbb.FixedTypedVector(uint32Vector.data(), uint32Vector.size());
    fbb.Finish();
    addNode(nodeID, {}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}
Identifier CircuitBuilder::addConstantNodeWithPayload(const std::vector<uint64_t> &uint64Vector, const std::string &annotations) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::UInt64, ir::SecurityLevel::Plaintext, {static_cast<long>(uint64Vector.size())});
    flexbuffers::Builder fbb;
    fbb.FixedTypedVector(uint64Vector.data(), uint64Vector.size());
    fbb.Finish();
    return addNode({}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, annotations);
}
void CircuitBuilder::addConstantNodeWithPayload(Identifier nodeID, const std::vector<uint64_t> &uint64Vector) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::UInt64, ir::SecurityLevel::Plaintext, {static_cast<long>(uint64Vector.size())});
    flexbuffers::Builder fbb;
    fbb.FixedTypedVector(uint64Vector.data(), uint64Vector.size());
    fbb.Finish();
    addNode(nodeID, {}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}
Identifier CircuitBuilder::addConstantNodeWithPayload(const std::vector<float> &floatVector, const std::string &annotations) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Float, ir::SecurityLevel::Plaintext, {static_cast<long>(floatVector.size())});
    flexbuffers::Builder fbb;
    fbb.FixedTypedVector(floatVector.data(), floatVector.size());
    fbb.Finish();
    return addNode({}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, annotations);
}
void CircuitBuilder::addConstantNodeWithPayload(Identifier nodeID, const std::vector<float> &floatVector) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Float, ir::SecurityLevel::Plaintext, {static_cast<long>(floatVector.size())});
    flexbuffers::Builder fbb;
    fbb.FixedTypedVector(floatVector.data(), floatVector.size());
    fbb.Finish();
    addNode(nodeID, {}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}
Identifier CircuitBuilder::addConstantNodeWithPayload(const std::vector<double> &doubleVector, const std::string &annotations) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Double, ir::SecurityLevel::Plaintext, {static_cast<long>(doubleVector.size())});
    flexbuffers::Builder fbb;
    fbb.FixedTypedVector(doubleVector.data(), doubleVector.size());
    fbb.Finish();
    return addNode({}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, annotations);
}
void CircuitBuilder::addConstantNodeWithPayload(Identifier nodeID, const std::vector<double> &doubleVector) {
    size_t datatypeIndex = addDataType(ir::PrimitiveType::Double, ir::SecurityLevel::Plaintext, {static_cast<long>(doubleVector.size())});
    flexbuffers::Builder fbb;
    fbb.FixedTypedVector(doubleVector.data(), doubleVector.size());
    fbb.Finish();
    addNode(nodeID, {}, {}, {}, ir::PrimitiveOperation::Constant, "", "", fbb.GetBuffer(), 1, {datatypeIndex}, "");
}

Identifier CircuitBuilder::addNodeWithCustomOperation(const std::string &customOperationName,
                                                      const std::vector<size_t> &inputDatatypes,
                                                      const std::vector<Identifier> &inputNodeIdentifiers,
                                                      const std::vector<size_t> &outputDatatypes) {
    return addNode(inputDatatypes, inputNodeIdentifiers, {}, ir::PrimitiveOperation::Custom, customOperationName, "", {}, outputDatatypes.size(), outputDatatypes, "");
}
void CircuitBuilder::addNodeWithCustomOperation(Identifier nodeID, const std::string &customOperationName,
                                                const std::vector<size_t> &inputDatatypes,
                                                const std::vector<Identifier> &inputNodeIdentifiers,
                                                const std::vector<size_t> &outputDatatypes) {
    addNode(nodeID, inputDatatypes, inputNodeIdentifiers, {}, ir::PrimitiveOperation::Custom, customOperationName, "", {}, outputDatatypes.size(), outputDatatypes, "");
}

Identifier CircuitBuilder::addNodeWithCustomOperation(const std::string &customOperationName,
                                                      const std::vector<Identifier> &inputNodeIdentifiers,
                                                      const std::vector<unsigned int> &inputNodeOffsets,
                                                      unsigned int numberOfOutputs) {
    return addNode({}, inputNodeIdentifiers, inputNodeOffsets, ir::PrimitiveOperation::Custom, customOperationName, "", {}, numberOfOutputs, {}, "");
}

unsigned int CircuitBuilder::getNumberOfOutputsForSplit(ir::PrimitiveType inputType) {
    using pt = ir::PrimitiveType;
    switch (inputType) {
        case pt::Int64:
        case pt::UInt64:
        case pt::Double:
            return 64;
        case pt::Int32:
        case pt::UInt32:
        case pt::Float:
            return 32;
        case pt::Int16:
        case pt::UInt16:
            return 16;
        case pt::UInt8:
        case pt::Int8:
            return 8;
        case pt::Bool:
            // trivial split is allowed nonetheless
            return 1;
        default: {
            std::string name = ir::EnumNamePrimitiveType(inputType);
            throw std::logic_error("Unknown Primitive Type as input for Split Node: " + name);
        }
    }
}

Identifier CircuitBuilder::addSplitNode(ir::PrimitiveType inputDataType, Identifier inputNodeID) {
    size_t inputDatatype = addDataType(inputDataType);
    size_t outputBooleanType = addDataType(ir::PrimitiveType::Bool);
    return addNode({inputDatatype}, {inputNodeID}, {/* input offsets */}, ir::PrimitiveOperation::Split, "", "", {/* payload */}, getNumberOfOutputsForSplit(inputDataType), {outputBooleanType}, "");
}

void CircuitBuilder::addSplitNode(Identifier nodeID, ir::PrimitiveType inputDataType, Identifier inputNodeID) {
    size_t inputDatatype = addDataType(inputDataType);
    size_t outputBooleanType = addDataType(ir::PrimitiveType::Bool);
    addNode(nodeID, {inputDatatype}, {inputNodeID}, {/* input offsets */}, ir::PrimitiveOperation::Split, "", "", {/* payload */}, getNumberOfOutputsForSplit(inputDataType), {outputBooleanType}, "");
}

Identifier CircuitBuilder::addSelectOffsetNode(Identifier inputNodeID, unsigned int inputOffset, const std::string &nodeAnnotations) {
    return addNode({}, {inputNodeID}, {inputOffset}, ir::PrimitiveOperation::SelectOffset, "", "", {}, 1, {}, nodeAnnotations);
}

void CircuitBuilder::addSelectOffsetNode(Identifier nodeID, Identifier inputNodeID, unsigned int inputOffset, const std::string &nodeAnnotations) {
    addNode(nodeID, {}, {inputNodeID}, {inputOffset}, ir::PrimitiveOperation::SelectOffset, "", "", {}, 1, {}, nodeAnnotations);
}

Identifier CircuitBuilder::addCallToSubcircuitNode(const std::vector<size_t> &input_datatypes,
                                                   const std::vector<Identifier> &input_identifiers,
                                                   const std::vector<unsigned int> &input_offsets,
                                                   const std::string &subcircuit_name,
                                                   const std::vector<size_t> &output_datatypes,
                                                   const std::string &node_annotations) {
    return addNode(input_datatypes, input_identifiers, input_offsets, ir::PrimitiveOperation::CallSubcircuit, "", subcircuit_name, {/* payload */}, output_datatypes.size(), output_datatypes, node_annotations);
}
void CircuitBuilder::addCallToSubcircuitNode(Identifier nodeID, const std::vector<size_t> &input_datatypes,
                                             const std::vector<Identifier> &input_identifiers,
                                             const std::vector<unsigned int> &input_offsets,
                                             const std::string &subcircuit_name,
                                             const std::vector<size_t> &output_datatypes,
                                             const std::string &node_annotations) {
    addNode(nodeID, input_datatypes, input_identifiers, input_offsets, ir::PrimitiveOperation::CallSubcircuit, "", subcircuit_name, {/* payload */}, output_datatypes.size(), output_datatypes, node_annotations);
}

Identifier CircuitBuilder::addCallToSubcircuitNode(const std::vector<Identifier> &input_identifiers, const std::string &subcircuit_name, const std::string &node_annotations) {
    return addNode({/* input datatypes */}, input_identifiers, {/* input offsets */}, ir::PrimitiveOperation::CallSubcircuit, "", subcircuit_name, {/* payload */}, 1, {/* output types */}, node_annotations);
}
void CircuitBuilder::addCallToSubcircuitNode(Identifier nodeID, const std::vector<Identifier> &input_identifiers, const std::string &subcircuit_name, const std::string &node_annotations) {
    addNode(nodeID, {/* input datatypes */}, input_identifiers, {/* input offsets */}, ir::PrimitiveOperation::CallSubcircuit, "", subcircuit_name, {/* payload */}, 1, {/* output types */}, node_annotations);
}

Identifier CircuitBuilder::addCallToSubcircuitNode(const std::vector<Identifier> &inputNodeIdentifiers,
                                                   const std::vector<unsigned int> &inputOffsets,
                                                   const std::string &subcircuitName,
                                                   const std::string &nodeAnnotations) {
    return addNode({/* input datatypes */}, inputNodeIdentifiers, inputOffsets, ir::PrimitiveOperation::CallSubcircuit, "", subcircuitName, {/* payload */}, 1, {/* output types */}, nodeAnnotations);
}
void CircuitBuilder::addCallToSubcircuitNode(Identifier nodeID,
                                             const std::vector<Identifier> &inputNodeIdentifiers,
                                             const std::vector<unsigned int> &inputOffsets,
                                             const std::string &subcircuitName,
                                             const std::string &nodeAnnotations) {
    addNode(nodeID, {/* input datatypes */}, inputNodeIdentifiers, inputOffsets, ir::PrimitiveOperation::CallSubcircuit, "", subcircuitName, {/* payload */}, 1, {/* output types */}, nodeAnnotations);
}

void CircuitBuilder::finish() {
    if (!finished_) {
        // prepare inputs for circuit flatbuffer
        auto nameString = circuitBuilder_.CreateString(name_);
        auto inputIdentifierVector = circuitBuilder_.CreateVector(inputIdentifiers_);
        auto outputIdentifierVector = circuitBuilder_.CreateVector(outputIdentifiers_);
        auto nodeVector = circuitBuilder_.CreateVector(nodes_);
        auto annotationString = circuitBuilder_.CreateString(annotations_);

        // read out already serialized input data types
        std::vector<flatbuffers::Offset<ir::DataTypeTable>> inputTypeOffsets;
        for (auto inputTypeIndex : inputDataTypes_) {
            inputTypeOffsets.push_back(dataTypes_.at(inputTypeIndex));
        }
        auto inputDataTypeVector = circuitBuilder_.CreateVector(inputTypeOffsets);

        // read out already serialized output data types
        std::vector<flatbuffers::Offset<ir::DataTypeTable>> outputTypeOffsets;
        for (auto outputTypeIndex : outputDataTypes_) {
            outputTypeOffsets.push_back(dataTypes_.at(outputTypeIndex));
        }
        auto outputDataTypeVector = circuitBuilder_.CreateVector(outputTypeOffsets);

        ir::CircuitTableBuilder circuitTableBuilder(circuitBuilder_);
        circuitTableBuilder.add_name(nameString);
        circuitTableBuilder.add_inputs(inputIdentifierVector);
        circuitTableBuilder.add_input_datatypes(inputDataTypeVector);
        circuitTableBuilder.add_outputs(outputIdentifierVector);
        circuitTableBuilder.add_output_datatypes(outputDataTypeVector);
        circuitTableBuilder.add_nodes(nodeVector);
        circuitTableBuilder.add_circuit_annotations(annotationString);
        auto finalCircuit = circuitTableBuilder.Finish();
        circuitBuilder_.Finish(finalCircuit);
        finished_ = true;
    }
}

void CircuitBuilder::finishAndWriteToFile(const std::string &pathToSaveBuffer) {
    namespace io = fuse::core::util::io;
    if (!finished_) {
        finish();
    }
    uint8_t *bufferPointer = circuitBuilder_.GetBufferPointer();
    long size = circuitBuilder_.GetSize();
    io::writeFlatBufferToBinaryFile(pathToSaveBuffer, bufferPointer, size);
}

uint8_t *CircuitBuilder::getSerializedCircuitBufferPointer() {
    return circuitBuilder_.GetBufferPointer();
}

flatbuffers::uoffset_t CircuitBuilder::getSerializedCircuitBufferSize() {
    return circuitBuilder_.GetSize();
}

/*
 * Module Builder
 */

CircuitBuilder *ModuleBuilder::addCircuit(const std::string &circuitName) {
    circuitBuilders_[circuitName] = std::make_unique<CircuitBuilder>(circuitName);
    return circuitBuilders_[circuitName].get();
}

CircuitBuilder *ModuleBuilder::getCircuitFromName(const std::string &circuitName) {
    return circuitBuilders_[circuitName].get();
}

CircuitBuilder *ModuleBuilder::getMainCircuit() {
    return getCircuitFromName(entryPoint_);
}

bool ModuleBuilder::containsCircuit(const std::string &circuitName) {
    return circuitBuilders_.contains(circuitName);
}

void ModuleBuilder::setEntryCircuitName(const std::string &circuitName) {
    entryPoint_ = circuitName;
}

void ModuleBuilder::addAnnotations(const std::string &annotations) {
    moduleAnnotations_ += annotations;
}

void ModuleBuilder::addSerializedCircuit(char *bufferPointer, size_t bufferSize) {
    auto circ = ir::GetCircuitTable(bufferPointer);
    auto name = circ->name();
    auto circuitBinary = moduleBuilder_.CreateVector(reinterpret_cast<uint8_t *>(bufferPointer), bufferSize);
    ir::CircuitTableBufferBuilder circuitTableBufferBuilder(moduleBuilder_);
    circuitTableBufferBuilder.add_circuit_buffer(circuitBinary);
    auto serializedData = circuitTableBufferBuilder.Finish();
    bool nu = serializedData.IsNull();
    serializedCircuits_.push_back(serializedData);
}

void ModuleBuilder::finish() {
    if (!finished_) {
        for (auto &circuitBuilder : circuitBuilders_) {
            CircuitBuilder *currentCircuit = circuitBuilder.second.get();
            currentCircuit->finish();
            auto circuitBinary = moduleBuilder_.CreateVector(currentCircuit->getSerializedCircuitBufferPointer(),
                                                             currentCircuit->getSerializedCircuitBufferSize());
            ir::CircuitTableBufferBuilder circuitTableBufferBuilder(moduleBuilder_);
            circuitTableBufferBuilder.add_circuit_buffer(circuitBinary);
            auto circuitTableBuffer = circuitTableBufferBuilder.Finish();
            serializedCircuits_.push_back(circuitTableBuffer);
        }

        auto entryPointString = moduleBuilder_.CreateString(entryPoint_);
        auto moduleAnnotationString = moduleBuilder_.CreateString(moduleAnnotations_);
        auto circuitVector = moduleBuilder_.CreateVector(serializedCircuits_);

        ir::ModuleTableBuilder moduleTableBuilder(moduleBuilder_);
        moduleTableBuilder.add_entry_point(entryPointString);
        moduleTableBuilder.add_module_annotations(moduleAnnotationString);
        moduleTableBuilder.add_circuits(circuitVector);
        auto finalModule = moduleTableBuilder.Finish();
        moduleBuilder_.Finish(finalModule);
        finished_ = true;
    }
}

void ModuleBuilder::finishAndWriteToFile(const std::string &pathToSaveBuffer) {
    namespace io = fuse::core::util::io;
    if (!finished_) {
        finish();
    }
    uint8_t *bufPointer = moduleBuilder_.GetBufferPointer();
    size_t bufSize = moduleBuilder_.GetSize();
    io::writeFlatBufferToBinaryFile(pathToSaveBuffer, bufPointer, bufSize);
}

void ModuleBuilder::finish(const std::string &pathToSaveBuffer) {
    finishAndWriteToFile(pathToSaveBuffer);
}

uint8_t *ModuleBuilder::getSerializedModuleBufferPointer() {
    return moduleBuilder_.GetBufferPointer();
}

flatbuffers::uoffset_t ModuleBuilder::getSerializedModuleBufferSize() {
    return moduleBuilder_.GetSize();
}

}  // namespace fuse::frontend
