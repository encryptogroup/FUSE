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
#include "BaseVisitor.h"

#include "ModuleWrapper.h"

namespace fuse::core {

/*
 * BaseReadOnlyVisitor Member Functions
 */

void BaseReadOnlyVisitor::visit(const core::DataTypeReadOnly& datatype) {}

void BaseReadOnlyVisitor::visit(const core::NodeReadOnly& node) {}

void BaseReadOnlyVisitor::visit(const core::CircuitReadOnly& circuit) {
    circuit.topologicalTraversal([=, this](core::NodeReadOnly& node) { this->visit(node); });
}

void BaseReadOnlyVisitor::visit(const core::ModuleReadOnly& module) {
    visit(*module.getEntryCircuit());
}

void BaseReadOnlyVisitor::visit(const core::VisitableReadable& visitable) { throw visitor_error("Top-Level visit method called, so the specialized method is missing for visitable object"); }

/*
 * BaseReadAndWriteVisitor Member Functions
 */
void BaseReadAndWriteVisitor::visit(core::DataTypeObjectWrapper& datatype) {}
void BaseReadAndWriteVisitor::visit(core::NodeObjectWrapper& node) {}
void BaseReadAndWriteVisitor::visit(core::CircuitObjectWrapper& circuit) {
    for (auto node : circuit) {
        visit(node);
    }
}
void BaseReadAndWriteVisitor::visit(core::ModuleObjectWrapper& module) {
    auto entryCircuit = module.getEntryCircuit();
    visit(entryCircuit);
}
void BaseReadAndWriteVisitor::visit(core::VisitableWriteable& visitable) { throw visitor_error("Top-Level visit method called, so the specialized method is missing for visitable object"); }

/*
 * BufferWrapperReadonlyVisitor Member Functions
 */
void BufferWrapperReadonlyVisitor::visit(const core::DataTypeBufferWrapper& datatype) {}

void BufferWrapperReadonlyVisitor::visit(const core::NodeBufferWrapper& node) {}

void BufferWrapperReadonlyVisitor::visit(const core::CircuitBufferWrapper& circuit) {
    for (auto nodeWrapper : circuit) {
        nodeWrapper.accept(*this);
    }
}

void BufferWrapperReadonlyVisitor::visit(const core::ModuleBufferWrapper& module) {
    for (auto circWrapper : module) {
        circWrapper.accept(*this);
    }
}

void BufferWrapperReadonlyVisitor::visit(const core::DataTypeReadOnly& datatype) {
    // datatype buffer
    if (typeid(datatype) == typeid(DataTypeBufferWrapper)) {
        visit(dynamic_cast<const DataTypeBufferWrapper&>(datatype));
    }
}

void BufferWrapperReadonlyVisitor::visit(const core::NodeReadOnly& node) {
    if (typeid(node) == typeid(NodeBufferWrapper)) {
        visit(dynamic_cast<const NodeBufferWrapper&>(node));
    }
}

void BufferWrapperReadonlyVisitor::visit(const core::CircuitReadOnly& circuit) {
    if (typeid(circuit) == typeid(CircuitBufferWrapper)) {
        visit(dynamic_cast<const CircuitBufferWrapper&>(circuit));
    }
}

void BufferWrapperReadonlyVisitor::visit(const core::ModuleReadOnly& module) {
    if (typeid(module) == typeid(ModuleBufferWrapper)) {
        visit(dynamic_cast<const ModuleBufferWrapper&>(module));
    }
}

/* Top-Level definition of visit method */
void BufferWrapperReadonlyVisitor::visit(const core::VisitableReadable& visitable) {
    throw visitor_error("Top-Level visit method called, so the specialized method is missing for visitable object");
}

}  // namespace fuse::core
