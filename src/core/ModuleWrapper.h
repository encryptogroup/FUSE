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

#ifndef FUSE_MODULEWRAPPER_H
#define FUSE_MODULEWRAPPER_H

#include <cstddef>
#include <iterator>
#include <span>
#include <unordered_set>

#include "module_generated.h"

namespace fuse::core {

namespace ir = fuse::core::ir;

/*
****************************************************************************************************************************************************
****************************************************************************************************************************************************
******************************************************** Interface Definitions
**********************************************************************
****************************************************************************************************************************************************
****************************************************************************************************************************************************
*/

/*
 *
 * Visitable Interfaces
 *
 */

struct ReadOnlyVisitor;
struct ReadAndWriteVisitor;

struct VisitableReadable {
  virtual ~VisitableReadable() = default;
  virtual void accept(ReadOnlyVisitor &visitor) const = 0;
};

struct VisitableWriteable {
  virtual ~VisitableWriteable() = default;
  virtual void accept(ReadAndWriteVisitor &visitor) = 0;
};

/*
 *
 * Visitor Interfaces
 *
 */

struct DataTypeReadOnly;
struct NodeReadOnly;
struct CircuitReadOnly;
struct ModuleReadOnly;

struct ReadOnlyVisitor {
  virtual ~ReadOnlyVisitor() = default;

  virtual void visit(const core::DataTypeReadOnly &datatype) = 0;
  virtual void visit(const core::NodeReadOnly &node) = 0;
  virtual void visit(const core::CircuitReadOnly &circuit) = 0;
  virtual void visit(const core::ModuleReadOnly &module) = 0;

  virtual void visit(const VisitableReadable &visitable) = 0;
};

struct DataTypeObjectWrapper;
struct NodeObjectWrapper;
struct CircuitObjectWrapper;
struct ModuleObjectWrapper;

struct ReadAndWriteVisitor {
  virtual ~ReadAndWriteVisitor() = default;

  virtual void visit(core::DataTypeObjectWrapper &datatype) = 0;
  virtual void visit(core::NodeObjectWrapper &node) = 0;
  virtual void visit(core::CircuitObjectWrapper &circuit) = 0;
  virtual void visit(core::ModuleObjectWrapper &module) = 0;

  virtual void visit(core::VisitableWriteable &visitable) = 0;
};

/*
 *
 * DataType Interface
 *
 */
struct DataTypeReadOnly : VisitableReadable {
  virtual ~DataTypeReadOnly() = default;

  virtual bool isPrimitiveType() const = 0;
  virtual bool isSecureType() const = 0;
  virtual ir::PrimitiveType getPrimitiveType() const = 0;
  virtual std::string getPrimitiveTypeName() const = 0;
  virtual ir::SecurityLevel getSecurityLevel() const = 0;
  virtual std::string getSecurityLevelName() const = 0;
  virtual std::string getDataTypeAnnotations() const = 0;
  virtual std::string
  getStringValueForAttribute(const std::string &attribute) const = 0;
  virtual std::span<const int64_t> getShape() const = 0;
};

/*
 *
 * Node Interface
 *
 */
struct NodeReadOnly : public VisitableReadable {
  using DataType = std::unique_ptr<DataTypeReadOnly>;
  virtual ~NodeReadOnly() = default;

  virtual bool isConstantNode() const = 0;
  virtual bool isNodeWithCustomOp() const = 0;
  virtual bool isSubcircuitNode() const = 0;
  virtual bool isLoopNode() const = 0;
  virtual bool isSplitNode() const = 0;
  virtual bool isMergeNode() const = 0;
  virtual bool isInputNode() const = 0;
  virtual bool isOutputNode() const = 0;
  virtual bool isUnaryNode() const = 0;
  virtual bool isBinaryNode() const = 0;
  virtual bool hasBooleanOperator() const = 0;
  virtual bool hasComparisonOperator() const = 0;
  virtual bool hasArithmeticOperator() const = 0;
  virtual bool usesInputOffsets() const = 0;
  virtual uint64_t getNodeID() const = 0;
  virtual ir::PrimitiveOperation getOperation() const = 0;
  virtual std::string getOperationName() const = 0;
  virtual std::string getCustomOperationName() const = 0;
  virtual std::string getSubCircuitName() const = 0;
  virtual std::string getNodeAnnotations() const = 0;
  virtual std::string
  getStringValueForAttribute(std::string attribute) const = 0;
  virtual std::span<const uint64_t> getInputNodeIDs() const = 0;
  virtual std::span<const uint32_t> getInputOffsets() const = 0;

  virtual DataType getInputDataTypeAt(size_t inputNumber) const = 0;
  virtual std::vector<DataType> getInputDataTypes() const = 0;
  virtual size_t getNumberOfInputs() const = 0;
  virtual DataType getOutputDataTypeAt(size_t inputNumber) const = 0;
  virtual std::vector<DataType> getOutputDataTypes() const = 0;
  virtual size_t getNumberOfOutputs() const = 0;
  virtual DataType getConstantType() const = 0;

  // whoever feels like it, may also access the flexbuffer directly
  virtual const flexbuffers::Reference getConstantFlexbuffer() const = 0;
  virtual bool getConstantBool() const = 0;
  virtual int8_t getConstantInt8() const = 0;
  virtual int16_t getConstantInt16() const = 0;
  virtual int32_t getConstantInt32() const = 0;
  virtual int64_t getConstantInt64() const = 0;
  virtual uint8_t getConstantUInt8() const = 0;
  virtual uint16_t getConstantUInt16() const = 0;
  virtual uint32_t getConstantUInt32() const = 0;
  virtual uint64_t getConstantUInt64() const = 0;
  virtual float getConstantFloat() const = 0;
  virtual double getConstantDouble() const = 0;
  virtual std::span<const uint8_t> getConstantBinary() const = 0;
  virtual std::vector<bool> getConstantBoolVector() const = 0;
  virtual std::vector<std::vector<bool>> getConstantBoolMatrix() const = 0;
  virtual std::vector<int8_t> getConstantInt8Vector() const = 0;
  virtual std::vector<int16_t> getConstantInt16Vector() const = 0;
  virtual std::vector<int32_t> getConstantInt32Vector() const = 0;
  virtual std::vector<int64_t> getConstantInt64Vector() const = 0;
  virtual std::vector<uint8_t> getConstantUInt8Vector() const = 0;
  virtual std::vector<uint16_t> getConstantUInt16Vector() const = 0;
  virtual std::vector<uint32_t> getConstantUInt32Vector() const = 0;
  virtual std::vector<uint64_t> getConstantUInt64Vector() const = 0;
  virtual std::vector<float> getConstantFloatVector() const = 0;
  virtual std::vector<double> getConstantDoubleVector() const = 0;
};

/*
 *
 * Circuit Interface
 *
 */
struct CircuitReadOnly : public VisitableReadable {
  using Node = std::unique_ptr<NodeReadOnly>;
  using DataType = std::unique_ptr<DataTypeReadOnly>;
  virtual ~CircuitReadOnly() = default;

  virtual std::string getName() const = 0;
  virtual std::string getCircuitAnnotations() const = 0;
  virtual std::string
  getStringValueForAttribute(const std::string &attribute) const = 0;

  virtual std::span<const uint64_t> getInputNodeIDs() const = 0;
  virtual std::vector<DataType> getInputDataTypes() const = 0;
  virtual size_t getNumberOfInputs() const = 0;

  virtual std::span<const uint64_t> getOutputNodeIDs() const = 0;
  virtual std::vector<DataType> getOutputDataTypes() const = 0;
  virtual size_t getNumberOfOutputs() const = 0;

  virtual Node getNodeWithID(uint64_t nodeID) const = 0;
  virtual size_t getNumberOfNodes() const = 0;

  virtual void
  topologicalTraversal(std::function<void(NodeReadOnly &)> func) const = 0;
};

/*
 *
 * Module Interface
 *
 */
struct ModuleReadOnly : public VisitableReadable {
  using Circuit = std::unique_ptr<CircuitReadOnly>;
  virtual ~ModuleReadOnly() = default;

  virtual std::string getEntryCircuitName() const = 0;
  virtual std::string getModuleAnnotations() const = 0;
  virtual std::string
  getStringValueForAttribute(std::string attribute) const = 0;

  virtual Circuit getCircuitWithName(const std::string &name) const = 0;
  virtual Circuit getEntryCircuit() const = 0;

  virtual std::vector<std::string> getAllCircuitNames() const = 0;
};

/*
****************************************************************************************************************************************************
****************************************************************************************************************************************************
********************************************************* Wrapper Class
*Declarations ***************************************************************
****************************************************************************************************************************************************
****************************************************************************************************************************************************
*/

/*
 *
 * DataType Wrappers
 *
 */

class DataTypeBufferWrapper : public DataTypeReadOnly {
private:
  const ir::DataTypeTable *data_type_;

public:
  explicit DataTypeBufferWrapper(const ir::DataTypeTable *data_type_flatbuffer)
      : data_type_(data_type_flatbuffer) {}

  virtual bool isPrimitiveType() const override;
  virtual bool isSecureType() const override;
  virtual ir::PrimitiveType getPrimitiveType() const override;
  virtual std::string getPrimitiveTypeName() const override;
  virtual ir::SecurityLevel getSecurityLevel() const override;
  virtual std::string getSecurityLevelName() const override;
  virtual std::string getDataTypeAnnotations() const override;
  virtual std::string
  getStringValueForAttribute(const std::string &attribute) const override;
  virtual std::span<const int64_t> getShape() const override;

  virtual void accept(ReadOnlyVisitor &visitor) const override {
    visitor.visit(*this);
  }
};

class DataTypeObjectWrapper : public DataTypeReadOnly,
                              public VisitableWriteable {
private:
  ir::DataTypeTableT *data_type_object_;

public:
  explicit DataTypeObjectWrapper(ir::DataTypeTableT *data_type_object)
      : data_type_object_(data_type_object) {}

  virtual bool isPrimitiveType() const override;
  virtual bool isSecureType() const override;
  virtual ir::PrimitiveType getPrimitiveType() const override;
  virtual std::string getPrimitiveTypeName() const override;
  virtual ir::SecurityLevel getSecurityLevel() const override;
  virtual std::string getSecurityLevelName() const override;
  virtual std::string getDataTypeAnnotations() const override;
  virtual std::string
  getStringValueForAttribute(const std::string &attribute) const override;
  virtual std::span<const int64_t> getShape() const override;

  // mutable: setters
  void setPrimitiveType(ir::PrimitiveType primitiveType);
  void setSecurityLevel(ir::SecurityLevel securityLevel);
  void setDataTypeAnnotations(const std::string &annotations);
  void setStringValueForAttribute(const std::string &attribute,
                                  const std::string &value);
  void setShape(std::span<const int64_t> shape);

  virtual void accept(ReadOnlyVisitor &visitor) const override {
    visitor.visit(*this);
  }
  virtual void accept(ReadAndWriteVisitor &visitor) override {
    visitor.visit(*this);
  }
};

/*
 *
 * Node Wrappers
 *
 */

class NodeBufferWrapper : public NodeReadOnly {
  using DataType = std::unique_ptr<DataTypeReadOnly>;

private:
  const ir::NodeTable *node_flatbuffer_;
  // this method is to unify the getConstantXYZVector implementations as they
  // all work the same
  template <typename Value> std::vector<Value> getConstantVector() const;

public:
  explicit NodeBufferWrapper(const ir::NodeTable *node_flatbuffer)
      : node_flatbuffer_(node_flatbuffer) {}

  virtual bool isConstantNode() const override;
  virtual bool isNodeWithCustomOp() const override;
  virtual bool isSubcircuitNode() const override;
  virtual bool isLoopNode() const override;
  virtual bool isSplitNode() const override;
  virtual bool isMergeNode() const override;
  virtual bool isInputNode() const override;
  virtual bool isOutputNode() const override;
  virtual bool isUnaryNode() const override;
  virtual bool isBinaryNode() const override;
  virtual bool hasBooleanOperator() const override;
  virtual bool hasComparisonOperator() const override;
  virtual bool hasArithmeticOperator() const override;
  virtual bool usesInputOffsets() const override;
  virtual uint64_t getNodeID() const override;
  virtual ir::PrimitiveOperation getOperation() const override;
  virtual std::string getOperationName() const override;
  virtual std::string getCustomOperationName() const override;
  virtual std::string getSubCircuitName() const override;
  virtual std::string getNodeAnnotations() const override;
  virtual std::string
  getStringValueForAttribute(std::string attribute) const override;
  virtual std::span<const uint64_t> getInputNodeIDs() const override;
  virtual std::span<const uint32_t> getInputOffsets() const override;
  virtual DataType getInputDataTypeAt(size_t inputNumber) const override;
  virtual std::vector<DataType> getInputDataTypes() const override;
  virtual size_t getNumberOfInputs() const override;
  virtual DataType getOutputDataTypeAt(size_t inputNumber) const override;
  virtual std::vector<DataType> getOutputDataTypes() const override;
  virtual size_t getNumberOfOutputs() const override;
  virtual DataType getConstantType() const override;

  // whoever feels like it, may also access the flexbuffer directly
  virtual const flexbuffers::Reference getConstantFlexbuffer() const override;
  virtual bool getConstantBool() const override;
  virtual int8_t getConstantInt8() const override;
  virtual int16_t getConstantInt16() const override;
  virtual int32_t getConstantInt32() const override;
  virtual int64_t getConstantInt64() const override;
  virtual uint8_t getConstantUInt8() const override;
  virtual uint16_t getConstantUInt16() const override;
  virtual uint32_t getConstantUInt32() const override;
  virtual uint64_t getConstantUInt64() const override;
  virtual float getConstantFloat() const override;
  virtual double getConstantDouble() const override;
  virtual std::span<const uint8_t> getConstantBinary() const override;
  virtual std::vector<bool> getConstantBoolVector() const override;
  virtual std::vector<std::vector<bool>> getConstantBoolMatrix() const override;
  virtual std::vector<int8_t> getConstantInt8Vector() const override;
  virtual std::vector<int16_t> getConstantInt16Vector() const override;
  virtual std::vector<int32_t> getConstantInt32Vector() const override;
  virtual std::vector<int64_t> getConstantInt64Vector() const override;
  virtual std::vector<uint8_t> getConstantUInt8Vector() const override;
  virtual std::vector<uint16_t> getConstantUInt16Vector() const override;
  virtual std::vector<uint32_t> getConstantUInt32Vector() const override;
  virtual std::vector<uint64_t> getConstantUInt64Vector() const override;
  virtual std::vector<float> getConstantFloatVector() const override;
  virtual std::vector<double> getConstantDoubleVector() const override;

  virtual void accept(ReadOnlyVisitor &visitor) const override {
    visitor.visit(*this);
  }
};

class NodeObjectWrapper : public NodeReadOnly, public VisitableWriteable {
  using DataType = std::unique_ptr<DataTypeReadOnly>;
  using MutableDataType = DataTypeObjectWrapper;

private:
  ir::NodeTableT *node_object_;

  template <typename Value> std::vector<Value> getConstantVector() const;

public:
  explicit NodeObjectWrapper(ir::NodeTableT *node_object)
      : node_object_(node_object) {}

  virtual bool isConstantNode() const override;
  virtual bool isNodeWithCustomOp() const override;
  virtual bool isSubcircuitNode() const override;
  virtual bool isLoopNode() const override;
  virtual bool isSplitNode() const override;
  virtual bool isMergeNode() const override;
  virtual bool isInputNode() const override;
  virtual bool isOutputNode() const override;
  virtual bool isUnaryNode() const override;
  virtual bool isBinaryNode() const override;
  virtual bool hasBooleanOperator() const override;
  virtual bool hasComparisonOperator() const override;
  virtual bool hasArithmeticOperator() const override;
  virtual bool usesInputOffsets() const override;
  virtual uint64_t getNodeID() const override;
  virtual ir::PrimitiveOperation getOperation() const override;
  virtual std::string getOperationName() const override;
  virtual std::string getCustomOperationName() const override;
  virtual std::string getSubCircuitName() const override;
  virtual std::string getNodeAnnotations() const override;
  virtual std::string
  getStringValueForAttribute(std::string attribute) const override;
  virtual std::span<const uint64_t> getInputNodeIDs() const override;
  virtual std::span<const uint32_t> getInputOffsets() const override;
  virtual DataType getInputDataTypeAt(size_t inputNumber) const override;
  virtual std::vector<DataType> getInputDataTypes() const override;
  virtual size_t getNumberOfInputs() const override;
  virtual DataType getOutputDataTypeAt(size_t inputNumber) const override;
  virtual std::vector<DataType> getOutputDataTypes() const override;
  virtual size_t getNumberOfOutputs() const override;
  virtual DataType getConstantType() const override;

  // whoever feels like it, may also access the flexbuffer directly
  virtual const flexbuffers::Reference getConstantFlexbuffer() const override;
  virtual bool getConstantBool() const override;
  virtual int8_t getConstantInt8() const override;
  virtual int16_t getConstantInt16() const override;
  virtual int32_t getConstantInt32() const override;
  virtual int64_t getConstantInt64() const override;
  virtual uint8_t getConstantUInt8() const override;
  virtual uint16_t getConstantUInt16() const override;
  virtual uint32_t getConstantUInt32() const override;
  virtual uint64_t getConstantUInt64() const override;
  virtual float getConstantFloat() const override;
  virtual double getConstantDouble() const override;
  virtual std::span<const uint8_t> getConstantBinary() const override;
  virtual std::vector<bool> getConstantBoolVector() const override;
  virtual std::vector<std::vector<bool>> getConstantBoolMatrix() const override;
  virtual std::vector<int8_t> getConstantInt8Vector() const override;
  virtual std::vector<int16_t> getConstantInt16Vector() const override;
  virtual std::vector<int32_t> getConstantInt32Vector() const override;
  virtual std::vector<int64_t> getConstantInt64Vector() const override;
  virtual std::vector<uint8_t> getConstantUInt8Vector() const override;
  virtual std::vector<uint16_t> getConstantUInt16Vector() const override;
  virtual std::vector<uint32_t> getConstantUInt32Vector() const override;
  virtual std::vector<uint64_t> getConstantUInt64Vector() const override;
  virtual std::vector<float> getConstantFloatVector() const override;
  virtual std::vector<double> getConstantDoubleVector() const override;

  // mutable: setters and non-const getters
  void setNodeID(uint64_t nodeID);
  void setPrimitiveOperation(ir::PrimitiveOperation primitiveOperation);
  void setCustomOperationName(const std::string &customOperationName);
  void setSubCircuitName(const std::string &subcircuitName);
  void setNodeAnnotations(const std::string &nodeAnnotations);
  void setStringValueForAttribute(const std::string &attribute,
                                  const std::string &value);

  void setInputNodeIDs(std::span<uint64_t> inputNodeIDs);
  void setInputOffsets(std::span<uint32_t> inputOffsets);
  void setNumberOfOutputs(uint32_t numberOfOutputs);
  void replaceInputBy(uint64_t prevInputID, uint64_t newInputID,
                      uint32_t prevOffset = 0, uint32_t newOffset = 0);

  void setConstantType(ir::PrimitiveType primitiveType,
                       std::span<const int64_t> shape = {});

  void setPayload(const std::vector<uint8_t> &finishedFlexbuffer);

  void setPayload(bool boolValue);
  void setPayload(uint64_t unsignedValue);
  void setPayload(int64_t intValue);
  void setPayload(float floatValue);
  void setPayload(double doubleValue);

  void setPayload(const std::vector<bool> &boolVector);
  void setPayload(const std::vector<uint64_t> &unsignedVector);
  void setPayload(const std::vector<int64_t> &intVector);
  void setPayload(const std::vector<float> &floatVector);
  void setPayload(const std::vector<double> &doubleVector);

  MutableDataType getInputDataTypeAt(size_t inputNumber);
  std::vector<MutableDataType> getInputDataTypes();
  MutableDataType getOutputDataTypeAt(size_t inputNumber);
  std::vector<MutableDataType> getOutputDataTypes();
  MutableDataType getConstantType();

  virtual void accept(ReadOnlyVisitor &visitor) const override {
    visitor.visit(*this);
  }
  virtual void accept(ReadAndWriteVisitor &visitor) override {
    visitor.visit(*this);
  }
};

/*
 *
 * Circuit
 *
 */

class CircuitBufferWrapper : public CircuitReadOnly {
  using DataType = std::unique_ptr<DataTypeReadOnly>;
  using Node = std::unique_ptr<NodeReadOnly>;

private:
  const fuse::core::ir::CircuitTable *circuit_flatbuffer_;

  struct NodeIterator {
    using FBNodeVectorIterator =
        flatbuffers::VectorConstIterator<flatbuffers::Offset<ir::NodeTable>,
                                         const ir::NodeTable *>;
    // Iterator tags
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = NodeBufferWrapper;
    using pointer = value_type *;
    using reference = value_type &;

    // Constructors
    NodeIterator(FBNodeVectorIterator it) : iterator_(it) {}

    NodeIterator(const NodeIterator &other) : iterator_(other.iterator_) {}

    NodeIterator &operator=(const NodeIterator &other) {
      iterator_ = other.iterator_;
      return *this;
    }

    NodeIterator &operator=(NodeIterator &&other) {
      iterator_ = other.iterator_;
      return *this;
    }

    friend bool operator==(const NodeIterator &a, const NodeIterator &b) {
      return a.iterator_ == b.iterator_;
    };
    friend bool operator!=(const NodeIterator &a, const NodeIterator &b) {
      return a.iterator_ != b.iterator_;
    };
    friend bool operator<(const NodeIterator &a, const NodeIterator &b) {
      return a.iterator_ < b.iterator_;
    };

    difference_type operator-(const NodeIterator &other) const {
      return iterator_ - other.iterator_;
    }

    // Note: similar to flabtuffers' iterator implementation, return type is
    // incompatible with the standard 'reference operator*()'
    value_type operator*() const {
      auto wrapper = NodeBufferWrapper(*iterator_);
      return wrapper;
    }

    // Note: similar to flabtuffers' iterator implementation, return type is
    // incompatible with the standard 'pointer operator->()'
    value_type operator->() const {
      auto wrapper = NodeBufferWrapper(iterator_.operator->());
      return wrapper;
    }

    NodeIterator &operator++() {
      iterator_++;
      return *this;
    }

    NodeIterator operator++(int) {
      NodeIterator tmp = *this;
      ++(*this);
      return tmp;
    }

    NodeIterator operator+(const uint32_t &offset) const {
      return NodeIterator(iterator_ + offset);
    }

    NodeIterator &operator+=(const uint32_t &offset) {
      iterator_ += offset;
      return *this;
    }

    NodeIterator &operator--() {
      iterator_--;
      return *this;
    }

    NodeIterator operator--(int) {
      NodeIterator temp(iterator_);
      iterator_--;
      return temp;
    }

    NodeIterator operator-(const uint32_t &offset) const {
      return NodeIterator(iterator_ - offset);
    }

    NodeIterator &operator-=(const uint32_t &offset) {
      iterator_ -= offset;
      return *this;
    }

  private:
    FBNodeVectorIterator iterator_;
  };

public:
  explicit CircuitBufferWrapper(const ir::CircuitTable *circuit_flatbuffer)
      : circuit_flatbuffer_(circuit_flatbuffer) {}
  explicit CircuitBufferWrapper(const uint8_t *serializedCircuitBufferPointer)
      : circuit_flatbuffer_(
            fuse::core::ir::GetCircuitTable(serializedCircuitBufferPointer)) {}

  // circuit name, annotations
  virtual std::string getName() const override;
  virtual std::string getCircuitAnnotations() const override;

  virtual std::string
  getStringValueForAttribute(const std::string &attribute) const override;
  virtual std::span<const uint64_t> getInputNodeIDs() const override;
  virtual std::vector<DataType> getInputDataTypes() const override;

  virtual size_t getNumberOfInputs() const override;
  virtual std::span<const uint64_t> getOutputNodeIDs() const override;
  virtual std::vector<DataType> getOutputDataTypes() const override;

  virtual size_t getNumberOfOutputs() const override;
  virtual Node getNodeWithID(uint64_t nodeID) const override;
  virtual size_t getNumberOfNodes() const override;

  NodeIterator begin() const {
    return NodeIterator(circuit_flatbuffer_->nodes()->begin());
  }
  NodeIterator end() const {
    return NodeIterator(circuit_flatbuffer_->nodes()->end());
  }

  virtual void accept(ReadOnlyVisitor &visitor) const override {
    visitor.visit(*this);
  }

  virtual void
  topologicalTraversal(std::function<void(NodeReadOnly &)> func) const {
    for (auto it = begin(), e = end(); it != e; ++it) {
      auto node = *it;
      func(node);
    }
  }
};

class CircuitObjectWrapper : public VisitableWriteable, public CircuitReadOnly {
  using Node = std::unique_ptr<NodeReadOnly>;
  using DataType = std::unique_ptr<DataTypeReadOnly>;
  using MutableNode = NodeObjectWrapper;
  using MutableDataType = DataTypeObjectWrapper;

private:
  ir::CircuitTableT *circuit_object_;

  struct NodeIterator {
    using iterator = std::vector<std::unique_ptr<ir::NodeTableT>>::iterator;
    // Iterator tags
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = NodeObjectWrapper;
    using pointer = value_type *;
    using reference = value_type &;

    // Constructors
    NodeIterator(iterator it) : iterator_(it) {}

    NodeIterator(const NodeIterator &other) : iterator_(other.iterator_) {}

    NodeIterator &operator=(const NodeIterator &other) {
      iterator_ = other.iterator_;
      return *this;
    }

    NodeIterator &operator=(NodeIterator &&other) {
      iterator_ = other.iterator_;
      return *this;
    }

    friend bool operator==(const NodeIterator &a, const NodeIterator &b) {
      return a.iterator_ == b.iterator_;
    };
    friend bool operator!=(const NodeIterator &a, const NodeIterator &b) {
      return a.iterator_ != b.iterator_;
    };
    friend bool operator<(const NodeIterator &a, const NodeIterator &b) {
      return a.iterator_ < b.iterator_;
    };

    difference_type operator-(const NodeIterator &other) const {
      return iterator_ - other.iterator_;
    }

    // Note: similar to flabtuffers' iterator implementation, return type is
    // incompatible with the standard 'reference operator*()'
    value_type operator*() const {
      auto wrapper = NodeObjectWrapper(iterator_.operator->()->get());
      return wrapper;
    }

    // Note: similar to flabtuffers' iterator implementation, return type is
    // incompatible with the standard 'pointer operator->()'
    value_type operator->() const {
      auto wrapper = NodeObjectWrapper(iterator_.operator->()->get());
      return wrapper;
    }

    NodeIterator &operator++() {
      iterator_++;
      return *this;
    }

    NodeIterator operator++(int) {
      NodeIterator tmp = *this;
      ++(*this);
      return tmp;
    }
    
    NodeIterator operator+(const uint32_t &offset) const {
      return NodeIterator(iterator_ + offset);
    }

    NodeIterator &operator+=(const uint32_t &offset) {
      iterator_ += offset;
      return *this;
    }

    NodeIterator &operator--() {
      iterator_--;
      return *this;
    }

    NodeIterator operator--(int) {
      NodeIterator temp(iterator_);
      iterator_--;
      return temp;
    }

    NodeIterator operator-(const uint32_t &offset) const {
      return NodeIterator(iterator_ - offset);
    }

    NodeIterator &operator-=(const uint32_t &offset) {
      iterator_ -= offset;
      return *this;
    }

  private:
    iterator iterator_;
  };

   public:
    explicit CircuitObjectWrapper(ir::CircuitTableT* circuit_object) : circuit_object_(circuit_object) {}

    virtual std::string getName() const override;
    virtual std::string getCircuitAnnotations() const override;

    virtual std::string getStringValueForAttribute(const std::string& attribute) const override;
    virtual std::span<const uint64_t> getInputNodeIDs() const override;
    virtual std::vector<DataType> getInputDataTypes() const override;

    virtual size_t getNumberOfInputs() const override;
    virtual std::span<const uint64_t> getOutputNodeIDs() const override;
    virtual std::vector<DataType> getOutputDataTypes() const override;

    virtual size_t getNumberOfOutputs() const override;
    virtual Node getNodeWithID(uint64_t nodeID) const override;
    virtual size_t getNumberOfNodes() const override;

    /* mutate : non-const getters and setters */
    void setName(const std::string& name);
    void setCircuitAnnotations(const std::string& annotations);
    void setStringValueForAttribute(const std::string& aribute, const std::string& value);
    MutableDataType getInputDataTypeAt(size_t inputNumber);
    std::vector<MutableDataType> getInputDataTypes();
    void setInputNodeIDs(std::span<uint64_t> inputNodeIDs);

    MutableDataType getOutputDataTypeAt(size_t inputNumber);
    std::vector<MutableDataType> getOutputDataTypes();

    void setOutputNodeIDs(std::span<uint64_t> outputNodeIDs);
    MutableNode getNodeWithID(uint64_t nodeID);

    /*
    TODO work in progress to replace nodes by a call node
    */
    // addNode with a mutable referene to it to configure it
    // Get next available identifier in the circuit
    uint64_t getNextID() const;
    /**
     * @brief Adds node with the next available ID at the back of the circuit.
     *
     * @return MutableNode a wrapper to the added node for further mutation.
     */
    MutableNode addNode();
    /**
     * @brief Adds a node with the next available ID at the given position inside the circuit.
     *
     * @param position position to insert the node into the circuit.
     * @return MutableNode a mutable wrapper to the newly added node for further mutation.
     */
    MutableNode addNode(long position);
    uint64_t replaceNodesBySubcircuit(CircuitReadOnly& subcircuit,
                                  // nodees that need to be deleted afterwards
                                  std::span<uint64_t> nodesToReplace,
                                  // subcircuit input to nodes in original circuit that contain the input values
                                  std::unordered_map<uint64_t, uint64_t> subcircuitInputToCircuitNode,
                                  // to circuit nodes that use this output as input
                                  std::unordered_map<uint64_t, std::vector<uint64_t>> subcircuitOutputToCircuitNodes,
                                  // maps subcircuit output to one of the nodes to replace
                                  std::unordered_map<uint64_t, uint64_t> subcircuitOutputToCircuitNode);
    void replaceNodesBySIMDNode(std::span<uint64_t> nodesToSimdify);
    void iterativelyRestoreTopologicalOrder(uint64_t nodeID, std::unordered_map<uint64_t, std::unordered_set<uint64_t>> nodeSuccessors);

    void removeNode(uint64_t nodeToDelete);
    void removeNodes(const std::unordered_set<uint64_t>& nodesToDelete);
    void removeNodesNotContainedIn(const std::unordered_set<uint64_t>& nodesToKeep);

    NodeIterator begin() const { return NodeIterator(circuit_object_->nodes.begin()); }
    NodeIterator end() const { return NodeIterator(circuit_object_->nodes.end()); }

    virtual void accept(ReadOnlyVisitor& visitor) const override { visitor.visit(*this); }
    virtual void accept(ReadAndWriteVisitor& visitor) override { visitor.visit(*this); }

    virtual void topologicalTraversal(std::function<void(NodeReadOnly&)> func) const {
        for (auto it = begin(), e = end(); it != e; ++it) {
            auto node = *it;
            func(node);
        }
    }  
};

/*
 *
 * Module
 *
 */

class ModuleBufferWrapper : public ModuleReadOnly {
  using Circuit = std::unique_ptr<CircuitReadOnly>;

  struct CircuitIterator {
    using FBCircuitBufferVectorIterator = flatbuffers::VectorConstIterator<
        flatbuffers::Offset<ir::CircuitTableBuffer>,
        const ir::CircuitTableBuffer *>;
    // Iterator tags
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = CircuitBufferWrapper;
    using pointer = value_type *;
    using reference = value_type &;

    // Constructors
    CircuitIterator(FBCircuitBufferVectorIterator it) : iterator_(it) {}

    CircuitIterator(const CircuitIterator &other)
        : iterator_(other.iterator_) {}

    CircuitIterator &operator=(const CircuitIterator &other) {
      iterator_ = other.iterator_;
      return *this;
    }

    CircuitIterator &operator=(CircuitIterator &&other) {
      iterator_ = other.iterator_;
      return *this;
    }

    friend bool operator==(const CircuitIterator &a, const CircuitIterator &b) {
      return a.iterator_ == b.iterator_;
    };
    friend bool operator!=(const CircuitIterator &a, const CircuitIterator &b) {
      return a.iterator_ != b.iterator_;
    };
    friend bool operator<(const CircuitIterator &a, const CircuitIterator &b) {
      return a.iterator_ < b.iterator_;
    };

    difference_type operator-(const CircuitIterator &other) const {
      return iterator_ - other.iterator_;
    }

    // Note: similar to flabtuffers' iterator implementation, return type is
    // incompatible with the standard 'reference operator*()'
    value_type operator*() const {
      auto wrapper =
          CircuitBufferWrapper((*iterator_)->circuit_buffer_nested_root());
      return wrapper;
    }

    // Note: similar to flabtuffers' iterator implementation, return type is
    // incompatible with the standard 'pointer operator->()'
    value_type operator->() const {
      auto wrapper = CircuitBufferWrapper(
          iterator_.operator->()->circuit_buffer_nested_root());
      return wrapper;
    }

    CircuitIterator &operator++() {
      iterator_++;
      return *this;
    }

    CircuitIterator operator++(int) {
      CircuitIterator tmp = *this;
      ++(*this);
      return tmp;
    }

    CircuitIterator operator+(const uint32_t &offset) const {
      return CircuitIterator(iterator_ + offset);
    }

    CircuitIterator &operator+=(const uint32_t &offset) {
      iterator_ += offset;
      return *this;
    }

    CircuitIterator &operator--() {
      iterator_--;
      return *this;
    }

    CircuitIterator operator--(int) {
      CircuitIterator temp(iterator_);
      iterator_--;
      return temp;
    }

    CircuitIterator operator-(const uint32_t &offset) const {
      return CircuitIterator(iterator_ - offset);
    }

    CircuitIterator &operator-=(const uint32_t &offset) {
      iterator_ -= offset;
      return *this;
    }

  private:
    FBCircuitBufferVectorIterator iterator_;
  };

private:
  const fuse::core::ir::ModuleTable *module_flatbuffer_;

public:
  explicit ModuleBufferWrapper(const ir::ModuleTable *module_flatbuffer)
      : module_flatbuffer_(module_flatbuffer) {}
  explicit ModuleBufferWrapper(const uint8_t *serializedCircuitBufferPointer)
      : module_flatbuffer_(
            fuse::core::ir::GetModuleTable(serializedCircuitBufferPointer)) {}

  virtual std::string getEntryCircuitName() const override;
  virtual std::string getModuleAnnotations() const override;
  virtual std::string
  getStringValueForAttribute(std::string attribute) const override;

  virtual Circuit getCircuitWithName(const std::string &name) const override;
  virtual Circuit getEntryCircuit() const override;

  virtual std::vector<std::string> getAllCircuitNames() const override;

  CircuitIterator begin() const {
    return CircuitIterator(module_flatbuffer_->circuits()->begin());
  }
  CircuitIterator end() const {
    return CircuitIterator(module_flatbuffer_->circuits()->end());
  }

  virtual void accept(ReadOnlyVisitor &visitor) const override {
    visitor.visit(*this);
  }
};

class ModuleObjectWrapper : public VisitableWriteable, public ModuleReadOnly {
  using Circuit = std::unique_ptr<CircuitReadOnly>;
  using MutableCircuit = CircuitObjectWrapper;

private:
  ir::ModuleTableT *module_object_;
  std::vector<std::unique_ptr<ir::CircuitTableT>> unpacked_circuits_;

public:
  explicit ModuleObjectWrapper(ir::ModuleTableT *module_object)
      : module_object_(module_object) {}

  virtual std::string getEntryCircuitName() const override;
  virtual std::string getModuleAnnotations() const override;
  virtual std::string
  getStringValueForAttribute(std::string attribute) const override;

  virtual Circuit getCircuitWithName(const std::string &name) const override;
  virtual Circuit getEntryCircuit() const override;

  virtual std::vector<std::string> getAllCircuitNames() const override;

  void setEntryCircuitName(const std::string &name);
  void setModuleAnnotations(const std::string &annotations);
  void setStringValueForAttribute(const std::string &attribute,
                                  const std::string &value);
  void removeCircuit(const std::string &name);

  MutableCircuit getCircuitWithName(const std::string &name);
  MutableCircuit getEntryCircuit();

  virtual void accept(ReadOnlyVisitor &visitor) const override {
    visitor.visit(*this);
  }
  virtual void accept(ReadAndWriteVisitor &visitor) override {
    visitor.visit(*this);
  }
};

} // namespace fuse::core
#endif /* FUSE_MODULEWRAPPER_H */
