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
#ifndef FUSE_PRIMITIVEOPERATIONTRAITS_HPP
#define FUSE_PRIMITIVEOPERATIONTRAITS_HPP

#include <math.h>

#include <bitset>
#include <vector>

#include "node_generated.h"

/**
 * @brief Defines template policies for every core::ir::PrimitiveOperation.
 *
 * Whenever sensible, the policies define an "accumulate" function, and/or an "apply" function.
 * As the signatures between accumulate and apply may differ,
 * the policies are not defined as struct templates
 * but as normal structs with static member templates.
 */

namespace fuse::core {

template <core::ir::PrimitiveOperation operation>
struct PrimitiveOperationPolicies;

/*
 * Operations that have both accumulate and apply definitions:
 * And, Xor, Not, Or, Add, Mul, Div, Sub, Neg
 */

template <>
struct PrimitiveOperationPolicies<core::ir::PrimitiveOperation::And> {
    template <typename T1, typename T2>
    static void accumulate(T1& total, const T2& value) { total &= value; };

    template <typename T1, typename T2 = T1,
              typename RT = std::conditional<sizeof(T1) >= sizeof(T2), T1, T2>>
    static RT apply(const T1& val1, const T2& val2) { return (val1 & val2); }
};
template <>
struct PrimitiveOperationPolicies<core::ir::PrimitiveOperation::Xor> {
    template <typename T1, typename T2>
    static void accumulate(T1& total, const T2& value) { total ^= value; };

    template <typename T1, typename T2 = T1,
              typename RT = std::conditional<sizeof(T1) >= sizeof(T2), T1, T2>>
    static RT apply(const T1& val1, const T2& val2) { return (val1 ^ val2); }
};
template <>
struct PrimitiveOperationPolicies<core::ir::PrimitiveOperation::Not> {
    template <typename T>
    static void accumulate(T& total) { total = !total; };

    template <typename T,
              typename RT = T>
    static RT apply(const T& val) { return !val; }
};
template <>
struct PrimitiveOperationPolicies<core::ir::PrimitiveOperation::Or> {
    template <typename T1, typename T2>
    static void accumulate(T1& total, const T2& value) { total |= value; };

    template <typename T1, typename T2 = T1,
              typename RT = std::conditional<sizeof(T1) >= sizeof(T2), T1, T2>>
    static RT apply(const T1& val1, const T2& val2) { return (val1 | val2); }
};

template <>
struct PrimitiveOperationPolicies<core::ir::PrimitiveOperation::Add> {
    template <typename T1, typename T2>
    static void accumulate(T1& total, const T2& value) { total += value; };

    template <typename T1, typename T2 = T1,
              typename RT = typename std::conditional<sizeof(T1) >= sizeof(T2), T1, T2>::type>
    static RT apply(const T1& val1, const T2& val2) { return (val1 + val2); }
};
template <>
struct PrimitiveOperationPolicies<core::ir::PrimitiveOperation::Mul> {
    template <typename T1, typename T2>
    static void accumulate(T1& total, const T2& value) { total *= value; };

    template <typename T1, typename T2 = T1,
              typename RT = typename std::conditional<sizeof(T1) >= sizeof(T2), T1, T2>::type>
    static RT apply(const T1& val1, const T2& val2) { return (val1 * val2); }
};
template <>
struct PrimitiveOperationPolicies<core::ir::PrimitiveOperation::Div> {
    template <typename T1, typename T2>
    static void accumulate(T1& total, const T2& value) { total /= value; };

    template <typename T1, typename T2 = T1,
              typename RT = typename std::conditional<sizeof(T1) >= sizeof(T2), T1, T2>::type>
    static RT apply(const T1& val1, const T2& val2) { return (val1 / val2); }
};
template <>
struct PrimitiveOperationPolicies<core::ir::PrimitiveOperation::Sub> {
    template <typename T1, typename T2>
    static void accumulate(T1& total, const T2& value) { total -= value; };

    template <typename T1, typename T2 = T1,
              typename RT = typename std::conditional<sizeof(T1) >= sizeof(T2), T1, T2>::type>
    static RT apply(const T1& val1, const T2& val2) { return val1 - val2; }
};
template <>
struct PrimitiveOperationPolicies<core::ir::PrimitiveOperation::Neg> {
    template <typename T>
    static void accumulate(T& total) { total = -total; };

    template <typename T,
              typename RT = T>
    static RT apply(const T& val) { return -val; }
};

/*
 * Operations that only have apply definitions:
 * Nand, Nor, Xnor, Gt, Ge, Lt, Le, Eq, Split, Merge
 */

template <>
struct PrimitiveOperationPolicies<core::ir::PrimitiveOperation::Nand> {
    template <typename T1, typename T2 = T1,
              typename RT = typename std::conditional<sizeof(T1) >= sizeof(T2), T1, T2>::type>
    static RT apply(const T1& val1, const T2& val2) { return !(val1 & val2); }
};
template <>
struct PrimitiveOperationPolicies<core::ir::PrimitiveOperation::Nor> {
    template <typename T1, typename T2 = T1,
              typename RT = typename std::conditional<sizeof(T1) >= sizeof(T2), T1, T2>::type>
    static RT apply(const T1& val1, const T2& val2) { return !(val1 | val2); }
};
template <>
struct PrimitiveOperationPolicies<core::ir::PrimitiveOperation::Xnor> {
    template <typename T1, typename T2 = T1,
              typename RT = typename std::conditional<sizeof(T1) >= sizeof(T2), T1, T2>::type>
    static RT apply(const T1& val1, const T2& val2) { return !(val1 ^ val2); }
};
template <>
struct PrimitiveOperationPolicies<core::ir::PrimitiveOperation::Gt> {
    template <typename T1, typename T2 = T1,
              typename RT = typename std::conditional<sizeof(T1) >= sizeof(T2), T1, T2>::type>
    static RT apply(const T1& val1, const T2& val2) { return (val1 > val2); }
};
template <>
struct PrimitiveOperationPolicies<core::ir::PrimitiveOperation::Ge> {
    template <typename T1, typename T2 = T1,
              typename RT = typename std::conditional<sizeof(T1) >= sizeof(T2), T1, T2>::type>
    static RT apply(const T1& val1, const T2& val2) { return (val1 >= val2); }
};
template <>
struct PrimitiveOperationPolicies<core::ir::PrimitiveOperation::Lt> {
    template <typename T1, typename T2 = T1,
              typename RT = typename std::conditional<sizeof(T1) >= sizeof(T2), T1, T2>::type>
    static RT apply(const T1& val1, const T2& val2) { return (val1 < val2); }
};
template <>
struct PrimitiveOperationPolicies<core::ir::PrimitiveOperation::Le> {
    template <typename T1, typename T2 = T1,
              typename RT = typename std::conditional<sizeof(T1) >= sizeof(T2), T1, T2>::type>
    static RT apply(const T1& val1, const T2& val2) { return (val1 <= val2); }
};
template <>
struct PrimitiveOperationPolicies<core::ir::PrimitiveOperation::Eq> {
    template <typename T1, typename T2 = T1,
              typename RT = typename std::conditional<sizeof(T1) >= sizeof(T2), T1, T2>::type>
    static RT apply(const T1& val1, const T2& val2) { return (val1 == val2); }
};
template <>
struct PrimitiveOperationPolicies<core::ir::PrimitiveOperation::Split> {
    template <typename T, size_t NumBits = sizeof(T) * 8>
    static std::bitset<NumBits> apply(T val) { return std::bitset<NumBits>(val); }
};
template <>
struct PrimitiveOperationPolicies<core::ir::PrimitiveOperation::Merge> {
    template <typename RT>
    static RT apply(std::vector<bool> values) {
        // bn-1 bn-2 .... b2 b1 b0
        RT res = RT();
        int currentExp = values.size() - 1;
        for (auto val : values) {
            if (val) {
                res += 1 << currentExp;
            }
            --currentExp;
        }
        return res;
    }

    template <typename RT, size_t NumBits>
    static RT apply(std::bitset<NumBits> values) { return static_cast<RT>(values.to_ullong()); }
};
template <>
struct PrimitiveOperationPolicies<core::ir::PrimitiveOperation::Constant> {
    template <typename T>
    static T apply(const T& val) { return val; }
};
template <>
struct PrimitiveOperationPolicies<core::ir::PrimitiveOperation::Mux> {
    template <typename T1, typename T2 = T1,
              typename Cond = bool,
              typename RT = typename std::conditional<sizeof(T1) >= sizeof(T2), T1, T2>::type>
    static RT apply(const Cond& condition, const T1& val1, const T2& val2) { return (condition ? val1 : val2); }
};

}  // namespace fuse::core

#endif /* FUSE_PRIMITIVEOPERATIONTRAITS_HPP */
