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

#ifndef FUSE_MODULEBUILDER_H
#define FUSE_MODULEBUILDER_H

#include <IOHandlers.h>

#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "module_generated.h"

namespace fuse::frontend {
typedef uint64_t Identifier;

using flatbuffers::FlatBufferBuilder;
using std::string, std::unordered_map;

namespace ir = fuse::core::ir;

class ModuleBuilder;

// TODO serialization of datatypes is a bit cumbersome to use,
//      maybe create callbacks for simple datatypes?

class CircuitBuilder {
    FlatBufferBuilder circuitBuilder_;
    std::vector<flatbuffers::Offset<ir::NodeTable>> nodes_;
    std::vector<flatbuffers::Offset<ir::DataTypeTable>> dataTypes_;
    std::unordered_set<Identifier> customIDs_;
    // all IDs up until (but not including) nextID_ are definitely assigned to a node
    Identifier nextID_ = 0;
    const std::string name_;
    std::vector<Identifier> inputIdentifiers_;
    std::vector<size_t> inputDataTypes_;
    std::vector<Identifier> outputIdentifiers_;
    std::vector<size_t> outputDataTypes_;
    std::string annotations_;

    bool finished_ = false;

    Identifier addNode(const std::vector<size_t> &input_datatypes,
                       const std::vector<Identifier> &input_identifiers,
                       const std::vector<unsigned int> &input_offsets,
                       ir::PrimitiveOperation operation,
                       const std::string &custom_operation_name,
                       const std::string &subcircuit_name,
                       const std::vector<unsigned char> &payload,
                       unsigned int num_of_outputs,
                       const std::vector<size_t> &output_datatypes,
                       const std::string &node_annotations);

    void addNode(Identifier id,
                 const std::vector<size_t> &input_datatypes,
                 const std::vector<Identifier> &input_identifiers,
                 const std::vector<unsigned int> &input_offsets,
                 ir::PrimitiveOperation operation,
                 const std::string &custom_operation_name,
                 const std::string &subcircuit_name,
                 const std::vector<unsigned char> &payload,
                 unsigned int num_of_outputs,
                 const std::vector<size_t> &output_datatypes,
                 const std::string &node_annotations);

    Identifier getNextID() {
        while (customIDs_.contains(nextID_)) {
            ++nextID_;
        }
        Identifier idToReturn = nextID_;
        ++nextID_;
        return idToReturn;
    }

    unsigned int getNumberOfOutputsForSplit(ir::PrimitiveType inputType);

   public:
    explicit CircuitBuilder(std::string name) : name_(std::move(name)),
                                                circuitBuilder_(FlatBufferBuilder(1024)){};

    CircuitBuilder(std::string name, std::string annotations) : name_(std::move(name)),
                                                                annotations_(std::move(annotations)),
                                                                circuitBuilder_(FlatBufferBuilder(1024)){};

    CircuitBuilder(std::string name, size_t builder_size) : name_(std::move(name)),
                                                            circuitBuilder_(FlatBufferBuilder(builder_size)){};

    size_t addDataType(ir::PrimitiveType primitiveType,
                       ir::SecurityLevel securityLevel = ir::SecurityLevel::Secure,
                       const std::vector<int64_t> &shape = {},
                       const std::string &data_type_annotations = "");

    void addAnnotations(const std::string &annotations);

    Identifier addInputNode(size_t inputType, const std::string &nodeAnnotations = "");
    void addInputNode(Identifier nodeID, size_t inputType, const std::string &nodeAnnotations = "");

    Identifier addInputNode(std::vector<size_t> inputTypes, const std::string &nodeAnnotations = "");
    void addInputNode(Identifier nodeID, std::vector<size_t> inputTypes, const std::string &nodeAnnotations = "");

    Identifier addOutputNode(size_t outputType,
                             const std::vector<Identifier> &inputNodeIdentifiers,
                             const std::vector<unsigned int> &inputOffsets = {});
    void addOutputNode(Identifier nodeID, size_t outputType,
                       const std::vector<Identifier> &inputNodeIdentifiers,
                       const std::vector<unsigned int> &inputOffsets = {});

    Identifier addOutputNode(std::vector<size_t> outputTypes,
                             const std::vector<Identifier> &inputNodeIdentifiers,
                             const std::vector<unsigned int> &inputOffsets = {});
    void addOutputNode(Identifier nodeID, std::vector<size_t> outputTypes,
                       const std::vector<Identifier> &inputNodeIdentifiers,
                       const std::vector<unsigned int> &inputOffsets = {});

    Identifier addConstantNodeWithPayload(bool payload);
    Identifier addConstantNodeWithPayload(int8_t payload);
    Identifier addConstantNodeWithPayload(int16_t payload);
    Identifier addConstantNodeWithPayload(int32_t payload);
    Identifier addConstantNodeWithPayload(int64_t payload);
    Identifier addConstantNodeWithPayload(uint8_t payload);
    Identifier addConstantNodeWithPayload(uint16_t payload);
    Identifier addConstantNodeWithPayload(uint32_t payload);
    Identifier addConstantNodeWithPayload(uint64_t payload);
    Identifier addConstantNodeWithPayload(float payload);
    Identifier addConstantNodeWithPayload(double payload);
    Identifier addConstantNodeWithBinaryPayload(const std::vector<unsigned char> &binaryPayload, const std::string &annotations = "");
    Identifier addConstantNodeWithPayload(const std::vector<bool> &boolVector, const std::string &annotations = "");
    Identifier addConstantNodeWithPayload(const std::vector<int8_t> &int8Vector, const std::string &annotations = "");
    Identifier addConstantNodeWithPayload(const std::vector<int16_t> &int16Vector, const std::string &annotations = "");
    Identifier addConstantNodeWithPayload(const std::vector<int32_t> &int32Vector, const std::string &annotations = "");
    Identifier addConstantNodeWithPayload(const std::vector<int64_t> &int64Vector, const std::string &annotations = "");
    Identifier addConstantNodeWithPayload(const std::vector<uint8_t> &uint8Vector, const std::string &annotations = "");
    Identifier addConstantNodeWithPayload(const std::vector<uint16_t> &uint16Vector, const std::string &annotations = "");
    Identifier addConstantNodeWithPayload(const std::vector<uint32_t> &uint32Vector, const std::string &annotations = "");
    Identifier addConstantNodeWithPayload(const std::vector<uint64_t> &uint64Vector, const std::string &annotations = "");
    Identifier addConstantNodeWithPayload(const std::vector<float> &floatVector, const std::string &annotations = "");
    Identifier addConstantNodeWithPayload(const std::vector<double> &doubleVector, const std::string &annotations = "");
    Identifier addConstantNodeWithPayload(const std::vector<std::vector<bool>> &boolMatrix, const std::string &annotations = "");

    void addConstantNodeWithPayload(Identifier nodeID, bool payload);
    void addConstantNodeWithPayload(Identifier nodeID, int8_t payload);
    void addConstantNodeWithPayload(Identifier nodeID, int16_t payload);
    void addConstantNodeWithPayload(Identifier nodeID, int32_t payload);
    void addConstantNodeWithPayload(Identifier nodeID, int64_t payload);
    void addConstantNodeWithPayload(Identifier nodeID, uint8_t payload);
    void addConstantNodeWithPayload(Identifier nodeID, uint16_t payload);
    void addConstantNodeWithPayload(Identifier nodeID, uint32_t payload);
    void addConstantNodeWithPayload(Identifier nodeID, uint64_t payload);
    void addConstantNodeWithPayload(Identifier nodeID, float payload);
    void addConstantNodeWithPayload(Identifier nodeID, double payload);
    void addConstantNodeWithBinaryPayload(Identifier nodeID, const std::vector<unsigned char> &binaryPayload);
    void addConstantNodeWithPayload(Identifier nodeID, const std::vector<bool> &boolVector);
    void addConstantNodeWithPayload(Identifier nodeID, const std::vector<int8_t> &int8Vector);
    void addConstantNodeWithPayload(Identifier nodeID, const std::vector<int16_t> &int16Vector);
    void addConstantNodeWithPayload(Identifier nodeID, const std::vector<int32_t> &int32Vector);
    void addConstantNodeWithPayload(Identifier nodeID, const std::vector<int64_t> &int64Vector);
    void addConstantNodeWithPayload(Identifier nodeID, const std::vector<uint8_t> &uint8Vector);
    void addConstantNodeWithPayload(Identifier nodeID, const std::vector<uint16_t> &uint16Vector);
    void addConstantNodeWithPayload(Identifier nodeID, const std::vector<uint32_t> &uint32Vector);
    void addConstantNodeWithPayload(Identifier nodeID, const std::vector<uint64_t> &uint64Vector);
    void addConstantNodeWithPayload(Identifier nodeID, const std::vector<float> &floatVector);
    void addConstantNodeWithPayload(Identifier nodeID, const std::vector<double> &doubleVector);

    /*
    Nodes with Custom operation: Specify the exact interface of the operation
     */
    Identifier addNodeWithCustomOperation(const std::string &customOperationName,
                                          const std::vector<size_t> &inputDatatypes,
                                          const std::vector<Identifier> &inputNodeIdentifiers,
                                          const std::vector<size_t> &outputDatatypes);
    void addNodeWithCustomOperation(Identifier nodeID, const std::string &customOperationName,
                                    const std::vector<size_t> &inputDatatypes,
                                    const std::vector<Identifier> &inputNodeIdentifiers,
                                    const std::vector<size_t> &outputDatatypes);

    Identifier addNodeWithCustomOperation(const std::string &customOperationName,
                                          const std::vector<Identifier> &inputNodeIdentifiers,
                                          const std::vector<unsigned int> &inputNodeOffsets,
                                          unsigned int numberOfOutputs = 1);

    /*
    Regular nodes: Just specify inputs, operation and offset if needed
     */
    Identifier addNode(ir::PrimitiveOperation operation,
                       const std::vector<Identifier> &inputNodeIdentifiers,
                       const std::vector<unsigned int> &inputOffsets = {},
                       const std::string &nodeAnnotations = "");
    Identifier addNodeWithNumberOfOutputs(ir::PrimitiveOperation operation,
                                          const std::vector<Identifier> &inputNodeIdentifiers,
                                          const std::vector<unsigned int> &inputOffsets = {},
                                          unsigned int numberOfOutputs = 1,
                                          const std::string &nodeAnnotations = "");
    void addNode(Identifier nodeID, ir::PrimitiveOperation operation,
                 const std::vector<Identifier> &inputNodeIdentifiers,
                 const std::vector<unsigned int> &inputOffsets = {});

    /*
    Split always splits into Boolean Outputs
     */
    Identifier addSplitNode(ir::PrimitiveType inputDataType, Identifier inputNodeID);
    void addSplitNode(Identifier nodeID, ir::PrimitiveType inputDataType, Identifier inputNodeID);

    Identifier addSelectOffsetNode(Identifier inputNodeID, unsigned int inputOffset, const std::string &nodeAnnotations = "");
    void addSelectOffsetNode(Identifier nodeID, Identifier inputNodeID, unsigned int inputOffset, const std::string &nodeAnnotations = "");
    /*
    Calls
     */
    Identifier addCallToSubcircuitNode(const std::vector<size_t> &inputDatatypes,
                                       const std::vector<Identifier> &inputNodeIdentifiers,
                                       const std::vector<unsigned int> &inputOffsets,
                                       const std::string &subcircuitName,
                                       const std::vector<size_t> &outputDatatypes,
                                       const std::string &nodeAnnotations = "");
    void addCallToSubcircuitNode(Identifier nodeID, const std::vector<size_t> &inputDatatypes,
                                 const std::vector<Identifier> &inputNodeIdentifiers,
                                 const std::vector<unsigned int> &inputOffsets,
                                 const std::string &subcircuitName,
                                 const std::vector<size_t> &outputDatatypes,
                                 const std::string &nodeAnnotations = "");
    Identifier addCallToSubcircuitNode(const std::vector<Identifier> &inputNodeIdentifiers,
                                       const std::string &subcircuitName,
                                       const std::string &nodeAnnotations = "");
    void addCallToSubcircuitNode(Identifier nodeID,
                                 const std::vector<Identifier> &inputNodeIdentifiers,
                                 const std::string &subcircuitName,
                                 const std::string &nodeAnnotations = "");
    Identifier addCallToSubcircuitNode(const std::vector<Identifier> &inputNodeIdentifiers,
                                       const std::vector<unsigned int> &inputOffsets,
                                       const std::string &subcircuitName,
                                       const std::string &nodeAnnotations = "");
    void addCallToSubcircuitNode(Identifier nodeID,
                                 const std::vector<Identifier> &inputNodeIdentifiers,
                                 const std::vector<unsigned int> &inputOffsets,
                                 const std::string &subcircuitName,
                                 const std::string &nodeAnnotations = "");

    void finishAndWriteToFile(const std::string &pathToSaveBuffer);
    void finish();

    uint8_t *getSerializedCircuitBufferPointer();
    flatbuffers::uoffset_t getSerializedCircuitBufferSize();
};

class ModuleBuilder {
    FlatBufferBuilder moduleBuilder_ = FlatBufferBuilder(1024);
    unordered_map<string, std::unique_ptr<CircuitBuilder>> circuitBuilders_;
    std::vector<flatbuffers::Offset<ir::CircuitTableBuffer>> serializedCircuits_;

    std::string entryPoint_ = "main";
    std::string moduleAnnotations_;
    bool finished_ = false;

   public:
    CircuitBuilder *addCircuit(const std::string &circuitName);

    CircuitBuilder *getCircuitFromName(const std::string &circuitName);

    CircuitBuilder *getMainCircuit();

    void addSerializedCircuit(char *bufferPointer, size_t bufferSize);

    bool containsCircuit(const std::string &circuitName);

    void setEntryCircuitName(const std::string &circuitName);

    void addAnnotations(const std::string &annotations);

    void finishAndWriteToFile(const std::string &pathToSaveBuffer);

    // this method is deprecated. Use finishAndWriteToFile instead
    [[deprecated]] void finish(const std::string &pathToSaveBuffer);

    void finish();

    uint8_t *getSerializedModuleBufferPointer();

    flatbuffers::uoffset_t getSerializedModuleBufferSize();
};

}  // namespace fuse::frontend

#endif  // FUSE_MODULEBUILDER_H
