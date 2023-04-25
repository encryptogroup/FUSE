// MIT License
//
// Copyright (c) 2019 Oleksandr Tkachenko, 2022 Nora Khayata
// Cryptography and Privacy Engineering Group (ENCRYPTO)
// TU Darmstadt, Germany
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef FUSE_MOTIONBMVECTORIZATION_H
#define FUSE_MOTIONBMVECTORIZATION_H

#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "common/common.h"
#include "motion_implementation.hpp"

std::vector<std::tuple<std::string, unsigned int, unsigned int>> circuit_candidates{
    {"int_add8_size", 8, 8},
    {"int_sub8_size", 8, 8},
    {"int_add8_depth", 8, 8},
    {"int_gt8_size", 8, 8},
    {"int_gt8_depth", 8, 8},
    {"int_sub8_depth", 8, 8},
    {"int_add16_size", 16, 16},
    {"int_sub16_size", 16, 16},
    {"int_gt16_size", 16, 16},
    {"int_gt16_depth", 16, 16},
    {"int_add16_depth", 16, 16},
    {"int_mul8_size", 8, 8},
    {"int_sub16_depth", 16, 16},
    {"int_mul8_depth", 8, 8},
    {"int_add32_size", 32, 32},
    {"int_sub32_size", 32, 32},
    {"int_gt32_size", 32, 32},
    {"int_gt32_depth", 32, 32},
    {"int_add32_depth", 32, 32},
    {"int_add64_size", 64, 64},
    {"int_div8_size", 8, 8},
    {"int_sub32_depth", 32, 32},
    {"int_div8_depth", 8, 8},
    {"int_sub64_size", 64, 64},
    {"int_gt64_size", 64, 64},
    {"int_gt64_depth", 64, 64},
    {"int_mul16_size", 16, 16},
    {"int_mul16_depth", 16, 16},
    {"int_add64_depth", 64, 64},
    {"int_sub64_depth", 64, 64},
    {"int_div16_size", 16, 16},
    {"int_div16_depth", 16, 16},
    {"FP-eq", 64, 64},
    {"FP-lt", 64, 64},
    {"FP-ceil", 64, 0},
    {"int_mul32_size", 32, 32},
    {"int_mul32_depth", 32, 32},
    {"int_div32_size", 32, 32},
    {"int_div32_depth", 32, 32},
    {"FP-f2i", 64, 0},
    {"FP-i2f", 64, 0},
    {"int_mul64_size", 64, 64},
    {"int_mul64_depth", 64, 64},
    {"int_div64_size", 64, 64},
    {"int_div64_depth", 64, 64},
    {"FP-add", 64, 64},
    {"aes_128", 128, 128},
    {"aes_192", 192, 128},
    {"FP-mul", 64, 64},
    {"aes_256", 256, 128},
    {"md5", 512, 0},
    {"sha_256", 512, 256},
    {"FP-div", 64, 64},
    {"Keccak_f", 1600, 0},
    {"FP-sqrt", 64, 0},
    {"sha_512", 1024, 512},
};

struct BenchmarkOutput {
    double unoptTime{0};
    double optTime{0};
    BenchmarkOutput() = default;
    BenchmarkOutput(double unopt, double opt) : unoptTime(unopt), optTime(opt) {}

    BenchmarkOutput& operator+=(BenchmarkOutput&& rhs) {
        this->unoptTime += rhs.unoptTime;
        this->optTime += rhs.optTime;
        return *this;
    }

    BenchmarkOutput& operator/=(size_t scale) {
        this->unoptTime /= scale;
        this->optTime /= scale;
        return *this;
    }

    std::string to_string() {
        std::stringstream ss;
        ss << unoptTime << ", " << optTime;
        return ss.str();
    }
};

struct CommunicationOutput {
    unsigned long unoptBytes{0};
    unsigned long optBytes{0};
    unsigned long unoptMsg{0};
    unsigned long optMsg{0};

    CommunicationOutput() = default;
    CommunicationOutput(unsigned long uB, unsigned long oB, unsigned long uM, unsigned long oM) : unoptBytes(uB), optBytes(oB), unoptMsg(uM), optMsg(oM) {}

    CommunicationOutput& operator+=(CommunicationOutput&& rhs) {
        this->unoptBytes += rhs.unoptBytes;
        this->optBytes += rhs.optBytes;
        this->unoptMsg += rhs.unoptMsg;
        this->optMsg += rhs.optMsg;
        return *this;
    }

    CommunicationOutput& operator/=(size_t scale) {
        this->unoptBytes /= scale;
        this->optBytes /= scale;
        this->unoptMsg /= scale;
        this->optMsg /= scale;
        return *this;
    }
};

constexpr std::array<encrypto::motion::MpcProtocol, 2>
    booleanProtocols{encrypto::motion::MpcProtocol::kBmr, encrypto::motion::MpcProtocol::kBooleanGmw};

constexpr size_t number_of_parties{2};

constexpr size_t number_of_executions{16};

#endif /* FUSE_MOTIONBMVECTORIZATION_H */
