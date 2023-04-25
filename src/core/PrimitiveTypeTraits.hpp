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
#ifndef FUSE_PRIMITIVETYPETRAITS_HPP
#define FUSE_PRIMITIVETYPETRAITS_HPP

#include "node_generated.h"

namespace fuse::core {
/*
 ****************************************************************************************************************************************************
 ****************************************************************************************************************************************************
 ***************************************************** Types : Magical Template Utility *************************************************************
 ****************************************************************************************************************************************************
 ****************************************************************************************************************************************************
 */
template <typename T>
core::ir::PrimitiveType primitiveTypeOf(T value) { throw std::logic_error("Needs template specialization!"); }
template <>
core::ir::PrimitiveType primitiveTypeOf<bool>(bool value) { return core::ir::PrimitiveType::Bool; }
template <>
core::ir::PrimitiveType primitiveTypeOf<uint8_t>(uint8_t value) { return core::ir::PrimitiveType::UInt8; }
template <>
core::ir::PrimitiveType primitiveTypeOf<uint16_t>(uint16_t value) { return core::ir::PrimitiveType::UInt16; }
template <>
core::ir::PrimitiveType primitiveTypeOf<uint32_t>(uint32_t value) { return core::ir::PrimitiveType::UInt32; }
template <>
core::ir::PrimitiveType primitiveTypeOf<uint64_t>(uint64_t value) { return core::ir::PrimitiveType::UInt64; }
template <>
core::ir::PrimitiveType primitiveTypeOf<int8_t>(int8_t value) { return core::ir::PrimitiveType::Int8; }
template <>
core::ir::PrimitiveType primitiveTypeOf<int16_t>(int16_t value) { return core::ir::PrimitiveType::Int16; }
template <>
core::ir::PrimitiveType primitiveTypeOf<int32_t>(int32_t value) { return core::ir::PrimitiveType::Int32; }
template <>
core::ir::PrimitiveType primitiveTypeOf<int64_t>(int64_t value) { return core::ir::PrimitiveType::Int64; }
template <>
core::ir::PrimitiveType primitiveTypeOf<float>(float value) { return core::ir::PrimitiveType::Float; }
template <>
core::ir::PrimitiveType primitiveTypeOf<double>(double value) { return core::ir::PrimitiveType::Double; }

template <core::ir::PrimitiveType type>
struct PrimitiveTypeTraits;

template <>
struct PrimitiveTypeTraits<core::ir::PrimitiveType::Bool> {
    using AccumulationType = bool;
    using PayloadType = bool;
    static constexpr size_t NumBits = 1;
};
template <>
struct PrimitiveTypeTraits<core::ir::PrimitiveType::UInt8> {
    using AccumulationType = uint8_t;
    using PayloadType = uint64_t;
    static constexpr size_t NumBits = 8;
};
template <>
struct PrimitiveTypeTraits<core::ir::PrimitiveType::UInt16> {
    using AccumulationType = uint16_t;
    using PayloadType = uint64_t;
    static constexpr size_t NumBits = 16;
};
template <>
struct PrimitiveTypeTraits<core::ir::PrimitiveType::UInt32> {
    using AccumulationType = uint32_t;
    using PayloadType = uint64_t;
    static constexpr size_t NumBits = 32;
};
template <>
struct PrimitiveTypeTraits<core::ir::PrimitiveType::UInt64> {
    using AccumulationType = uint64_t;
    using PayloadType = uint64_t;
    static constexpr size_t NumBits = 64;
};
template <>
struct PrimitiveTypeTraits<core::ir::PrimitiveType::Int8> {
    using AccumulationType = int8_t;
    using PayloadType = int64_t;
    static constexpr size_t NumBits = 8;
};
template <>
struct PrimitiveTypeTraits<core::ir::PrimitiveType::Int16> {
    using AccumulationType = int16_t;
    using PayloadType = int64_t;
    static constexpr size_t NumBits = 16;
};
template <>
struct PrimitiveTypeTraits<core::ir::PrimitiveType::Int32> {
    using AccumulationType = int32_t;
    using PayloadType = int64_t;
    static constexpr size_t NumBits = 32;
};
template <>
struct PrimitiveTypeTraits<core::ir::PrimitiveType::Int64> {
    using AccumulationType = int64_t;
    using PayloadType = int64_t;
    static constexpr size_t NumBits = 64;
};
template <>
struct PrimitiveTypeTraits<core::ir::PrimitiveType::Float> {
    using AccumulationType = float;
    using PayloadType = float;
    static constexpr size_t NumBits = 32;
};
template <>
struct PrimitiveTypeTraits<core::ir::PrimitiveType::Double> {
    using AccumulationType = double;
    using PayloadType = double;
    static constexpr size_t NumBits = 64;
};

}  // namespace fuse::core

#endif /* FUSE_PRIMITIVETYPETRAITS_HPP */
