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

#include "IR.h"

#include "IOHandlers.h"
#include "circuit_generated.h"

namespace fuse::core {

/*
 * CircuitContext Member Functions
 */

CircuitContext CircuitContext::createCopy() {
    CircuitContext copy;
    copy.binary_size = binary_size;
    copy.is_unpacked = is_unpacked;

    // copy flatbuffer data
    std::copy(circuit_flatbuffer_data_.begin(), circuit_flatbuffer_data_.end(), std::back_inserter(copy.circuit_flatbuffer_data_));

    // copy object data
    copy.circuit_unpacked_data_ = std::make_unique<fuse::core::ir::CircuitTableT>(fuse::core::ir::CircuitTableT(*circuit_unpacked_data_));
    return copy;
}

CircuitBufferWrapper CircuitContext::readCircuitFromFile(const std::string& circuitPath) {
    // reset unpacked status to false
    is_unpacked = false;

    // delete unpacked circuit, if there was one
    circuit_unpacked_data_.reset();

    // read in new circuit flatbuffer from the given path
    circuit_flatbuffer_data_ = util::io::readFlatBufferFromBinary(circuitPath);

    // return read-only buffer wrapper for circuit flatbuffer data
    return CircuitBufferWrapper(ir::GetCircuitTable(circuit_flatbuffer_data_.data()));
}

void CircuitContext::writeCircuitToFile(const std::string& pathToWrite) {
    if (!is_unpacked) {
        util::io::writeFlatBufferToBinaryFile(pathToWrite, circuit_flatbuffer_data_);
    } else {
        flatbuffers::FlatBufferBuilder fbb;
        const ir::CircuitTableT* ptr = circuit_unpacked_data_.get();
        fbb.Finish(ir::CircuitTable::Pack(fbb, ptr));
        flatbuffers::SaveFile(pathToWrite.data(), reinterpret_cast<char*>(fbb.GetBufferPointer()), fbb.GetSize(), true);
    }
}

std::unique_ptr<core::CircuitReadOnly> CircuitContext::getReadOnlyCircuit() {
    if (!is_unpacked) {
        return std::make_unique<CircuitBufferWrapper>(ir::GetCircuitTable(circuit_flatbuffer_data_.data()));
    } else {
        return std::make_unique<CircuitObjectWrapper>(circuit_unpacked_data_.get());
    }
}

CircuitBufferWrapper CircuitContext::getCircuitBufferWrapper() {
    assert(!is_unpacked);
    return CircuitBufferWrapper(ir::GetCircuitTable(circuit_flatbuffer_data_.data()));
}

CircuitObjectWrapper CircuitContext::getMutableCircuitWrapper() {
    if (!is_unpacked) {
        is_unpacked = true;
        circuit_unpacked_data_.reset(ir::GetCircuitTable(circuit_flatbuffer_data_.data())->UnPack());
        // clear underlying flatbuffers data as this should not be used anymore anyway
        circuit_flatbuffer_data_.clear();
    }

    return CircuitObjectWrapper(circuit_unpacked_data_.get());
}

void CircuitContext::packCircuit() {
    if (is_unpacked) {
        is_unpacked = false;
        circuit_unpacked_data_->input_datatypes;
        // Serialize into new flatbuffer.
        flatbuffers::FlatBufferBuilder fbb;
        fbb.Finish(fuse::core::ir::CircuitTable::Pack(fbb, circuit_unpacked_data_.get()));
        circuit_flatbuffer_data_.assign(fbb.GetBufferPointer(), fbb.GetBufferPointer() + fbb.GetSize());
        circuit_unpacked_data_.reset(nullptr);
    }
}

void CircuitContext::reset() {
    is_unpacked = false;
    circuit_unpacked_data_.reset();
    circuit_flatbuffer_data_.clear();
}

/*
 * ModuleContext Member Functions
 */

ModuleBufferWrapper ModuleContext::readModuleFromFile(const std::string& modulePath) {
    // reset unpacked status to false
    is_unpacked = false;

    // delete unpacked module, if there was one
    module_unpacked_data_.reset();

    // read in new module flatbuffer from the given path
    module_flatbuffer_data_ = util::io::readFlatBufferFromBinary(modulePath);

    // return read-only module wrapper for module flatbuffer data
    return ModuleBufferWrapper(ir::GetModuleTable(module_flatbuffer_data_.data()));
}

void ModuleContext::writeModuleToFile(const std::string& pathToWrite) {
    if (!is_unpacked) {
        util::io::writeFlatBufferToBinaryFile(pathToWrite, module_flatbuffer_data_);
    } else {
        // pack first into new flatbuffer, then serialize flatbuffer
        flatbuffers::FlatBufferBuilder fbb;
        const ir::ModuleTableT* ptr = module_unpacked_data_.get();
        fbb.Finish(ir::ModuleTable::Pack(fbb, ptr));
        flatbuffers::SaveFile(pathToWrite.data(), reinterpret_cast<char*>(fbb.GetBufferPointer()), fbb.GetSize(), true);
    }
}

std::unique_ptr<core::ModuleReadOnly> ModuleContext::getReadOnlyModule() const {
    if (!is_unpacked) {
        const core::ir::ModuleTable* module_flatbuffer = ir::GetModuleTable(module_flatbuffer_data_.data());
        return std::make_unique<core::ModuleBufferWrapper>(module_flatbuffer);
    } else {
        return std::make_unique<ModuleObjectWrapper>(module_unpacked_data_.get());
    }
}

ModuleBufferWrapper ModuleContext::getModuleBufferWrapper() const {
    assert(!is_unpacked);
    return ModuleBufferWrapper(ir::GetModuleTable(module_flatbuffer_data_.data()));
}

ModuleObjectWrapper ModuleContext::getMutableModuleWrapper() {
    if (!is_unpacked) {
        is_unpacked = true;
        module_unpacked_data_.reset(ir::GetModuleTable(module_flatbuffer_data_.data())->UnPack());
        // clear underlying flatbuffers data as this should not be used anymore anyway
        module_flatbuffer_data_.clear();
    }
    return ModuleObjectWrapper(module_unpacked_data_.get());
}

void ModuleContext::reset() {
    is_unpacked = false;
    module_unpacked_data_.reset();
    module_flatbuffer_data_.clear();
}

}  // namespace fuse::core
