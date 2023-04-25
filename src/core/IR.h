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

#ifndef FUSE_IR_H
#define FUSE_IR_H

#include "ModuleBuilder.h"
#include "ModuleWrapper.h"

namespace fuse::core {

class CircuitContext {
   protected:
    std::vector<char> circuit_flatbuffer_data_;
    std::unique_ptr<ir::CircuitTableT> circuit_unpacked_data_;

    bool is_unpacked = false;
    std::size_t binary_size;

   public:
    explicit CircuitContext(frontend::CircuitBuilder& circuitBuilder) {
        circuitBuilder.finish();
        char* bufferPtr = reinterpret_cast<char*>(circuitBuilder.getSerializedCircuitBufferPointer());
        long bufferSize = circuitBuilder.getSerializedCircuitBufferSize();
        // construct vector from serialized data
        circuit_flatbuffer_data_ = std::vector<char>{bufferPtr, bufferPtr + bufferSize};
        binary_size = sizeof(std::vector<char>) + (sizeof(char) * circuit_flatbuffer_data_.size());
    }

    CircuitContext() = default;

    CircuitContext createCopy();

    // read from file
    CircuitBufferWrapper readCircuitFromFile(const std::string& pathToRead);

    // write to file
    void writeCircuitToFile(const std::string& pathToWrite);

    // get readonly reference
    std::unique_ptr<core::CircuitReadOnly> getReadOnlyCircuit();

    // get buffer wrapper
    CircuitBufferWrapper getCircuitBufferWrapper();

    // get mutable reference -> unpack first if not already done
    CircuitObjectWrapper getMutableCircuitWrapper();

    void packCircuit();

    std::size_t getBinarySize() { return binary_size; }

    char* getBufferPointer() { return circuit_flatbuffer_data_.data(); }

    std::size_t getBufferSize() { return circuit_flatbuffer_data_.size(); }

    // delete underlying data explicitly
    void reset();
};

class ModuleContext {
   private:
    std::vector<char> module_flatbuffer_data_{};
    std::unique_ptr<ir::ModuleTableT> module_unpacked_data_;

    bool is_unpacked{false};
    std::size_t binary_size{0};

   public:
    explicit ModuleContext(frontend::ModuleBuilder& moduleBuilder) {
        moduleBuilder.finish();
        char* bufferPtr = reinterpret_cast<char*>(moduleBuilder.getSerializedModuleBufferPointer());
        long bufferSize = moduleBuilder.getSerializedModuleBufferSize();
        // construct vector from serialized data
        module_flatbuffer_data_ = std::vector<char>{bufferPtr, bufferPtr + bufferSize};
        binary_size = sizeof(std::vector<char>) + (sizeof(char) * module_flatbuffer_data_.size());
    }

    ModuleContext() = default;

    // read from file
    ModuleBufferWrapper readModuleFromFile(const std::string& pathToRead);

    // write to file
    void writeModuleToFile(const std::string& pathToWrite);

    // get readonly reference
    std::unique_ptr<core::ModuleReadOnly> getReadOnlyModule() const;

    // get buffer wrapper
    ModuleBufferWrapper getModuleBufferWrapper() const;

    // get mutable reference - unpack
    ModuleObjectWrapper getMutableModuleWrapper();

    char* getBufferPointer() { return module_flatbuffer_data_.data(); }

    std::size_t getBufferSize() { return module_flatbuffer_data_.size(); }

    // delete underlying data explicitly
    void reset();
};

}  // namespace fuse::core

#endif  // FUSE_IR_H
