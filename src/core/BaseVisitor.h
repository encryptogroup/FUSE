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
#ifndef FUSE_BASEVISITOR_H
#define FUSE_BASEVISITOR_H

#include <stdexcept>

#include "ModuleWrapper.h"

namespace fuse::core {

class visitor_error : std::logic_error {
    using std::logic_error::logic_error;
};

struct BaseReadOnlyVisitor : public ReadOnlyVisitor {
    virtual void visit(const core::DataTypeReadOnly& datatype) override;
    virtual void visit(const core::NodeReadOnly& node) override;
    virtual void visit(const core::CircuitReadOnly& circuit) override;
    virtual void visit(const core::ModuleReadOnly& module) override;

    /* Top-Level definition of visit method */
    virtual void visit(const core::VisitableReadable& visitable) override;
};

struct BaseReadAndWriteVisitor : public ReadAndWriteVisitor {
    virtual void visit(core::DataTypeObjectWrapper& datatype) override;
    virtual void visit(core::NodeObjectWrapper& node) override;
    virtual void visit(core::CircuitObjectWrapper& circuit) override;
    virtual void visit(core::ModuleObjectWrapper& module) override;

    /* Top-Level definition of visit method */
    virtual void visit(core::VisitableWriteable& visitable) override;
};

struct BufferWrapperReadonlyVisitor : public ReadOnlyVisitor {
    virtual void visit(const core::DataTypeBufferWrapper& datatype);
    virtual void visit(const core::NodeBufferWrapper& node);
    virtual void visit(const core::CircuitBufferWrapper& circuit);
    virtual void visit(const core::ModuleBufferWrapper& module);

    virtual void visit(const core::DataTypeReadOnly& datatype) override;
    virtual void visit(const core::NodeReadOnly& node) override;
    virtual void visit(const core::CircuitReadOnly& circuit) override;
    virtual void visit(const core::ModuleReadOnly& module) override;

    /* Top-Level definition of visit method */
    virtual void visit(const core::VisitableReadable& visitable) override;
};

}  // namespace fuse::core

#endif /* FUSE_BASEVISITOR_H */
