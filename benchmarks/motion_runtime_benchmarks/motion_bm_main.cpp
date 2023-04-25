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

#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <regex>
#include <tuple>

// FUSE
#include "BristolFrontend.h"
#include "DOTBackend.h"
#include "InstructionVectorization.h"
#include "MOTIONBackend.h"
#include "MOTIONFrontend.h"

// Benchmark
#include "common/common.h"
#include "motion_vectorization_bm.h"

// MOTION
#include "motion_implementation.hpp"
#include "statistics/analysis.h"
#include "statistics/run_time_statistics.h"

template <encrypto::motion::MpcProtocol P>
BenchmarkOutput execBenchmark(const std::string& circuit_name,
                              size_t input_size_0,  // party 0
                              size_t input_size_1,  // party 1
                              boost::program_options::variables_map& user_options,
                              const std::string& vectorizedPath);

/*
*********************************** main: executed by each party *********************************
*/
int main(int ac, char* av[]) {
    namespace mo = encrypto::motion;
    namespace be = fuse::benchmarks;
    auto [user_options, help_flag] = ParseProgramOptions(ac, av);
    // if help flag is set - print allowed command line arguments and exit
    if (help_flag) return EXIT_SUCCESS;

    /*
     * Benchmark
     */


    std::fstream out(be::kOutputPath + "motion_exec_log_lan.txt", std::ios::in | std::ios::out | std::ios::app);
    out.seekg(0, std::ios::end);
    if (out.tellg() == 0) {
        // file is emtpy -> add first line
        out << "Circuit, Execution before Optimization (ms), Execution after Optimization (ms), Improvement"
            << std::endl;
    }

    // // BMR
    // out << "------------- [BMR] -------------" << std::endl;
    // for (auto candidate : circuit_candidates) {
    //     BenchmarkOutput bmr;

    //     for (int i = 0; i < number_of_executions; ++i) {
    //         bmr += execBenchmark<mo::MpcProtocol::kBmr>(
    //             std::get<0>(candidate), std::get<1>(candidate), std::get<2>(candidate), user_options, be::kPathToGreedyVect);
    //     }
    //     // times
    //     bmr /= number_of_executions;
    //     out << std::get<0>(candidate) << be::kSep
    //         << bmr.to_string() << be::kSep
    //         << std::endl;
    // }

    // Boolean GMW
    out << "------------- [Boolean GMW] -------------" << std::endl;
    out << "Circuit, Execution before Optimization (ms), Execution after Greedy (ms), Execution after (8) (ms), Exec after 16 (ms), Exec after 32 (ms), Exec after 64 (ms)" << std::endl;
    // out << "Circuit, Bytes before opt, Bytes Greedy, Bytes (8) (ms), Bytes 16 (ms), Bytes 32 (ms), Bytes 64 (ms), #Msg before, #Msg Greedy, #Msg 8, #Msg 16, #Msg 32, #Msg 64" << std::endl;
    for (auto candidate : circuit_candidates) {
        BenchmarkOutput greedy;
        BenchmarkOutput vect8;
        BenchmarkOutput vect16;
        BenchmarkOutput vect32;
        BenchmarkOutput vect64;

        for (int i = 0; i < number_of_executions; ++i) {
            greedy += execBenchmark<mo::MpcProtocol::kBooleanGmw>(
                std::get<0>(candidate), std::get<1>(candidate), std::get<2>(candidate), user_options, be::kPathToGreedyVect);
        }
        for (int i = 0; i < number_of_executions; ++i) {
            vect8 += execBenchmark<mo::MpcProtocol::kBooleanGmw>(
                std::get<0>(candidate), std::get<1>(candidate), std::get<2>(candidate), user_options, be::kPathToVect8);
        }
        for (int i = 0; i < number_of_executions; ++i) {
            vect16 += execBenchmark<mo::MpcProtocol::kBooleanGmw>(
                std::get<0>(candidate), std::get<1>(candidate), std::get<2>(candidate), user_options, be::kPathToVect16);
        }
        for (int i = 0; i < number_of_executions; ++i) {
            vect32 += execBenchmark<mo::MpcProtocol::kBooleanGmw>(
                std::get<0>(candidate), std::get<1>(candidate), std::get<2>(candidate), user_options, be::kPathToVect32);
        }
        for (int i = 0; i < number_of_executions; ++i) {
            vect64 += execBenchmark<mo::MpcProtocol::kBooleanGmw>(
                std::get<0>(candidate), std::get<1>(candidate), std::get<2>(candidate), user_options, be::kPathToVect64);
        }

        // times
        greedy /= number_of_executions;
        vect8 /= number_of_executions;
        vect16 /= number_of_executions;
        vect32 /= number_of_executions;
        vect64 /= number_of_executions;

        out << std::get<0>(candidate) << be::kSep
            << greedy.unoptTime << be::kSep
            << greedy.optTime << be::kSep
            << vect8.optTime << be::kSep
            << vect16.optTime << be::kSep
            << vect32.optTime << be::kSep
            << vect64.optTime << be::kSep
            << std::endl;
    }

    // [DONE]
    return EXIT_SUCCESS;
}

double computeAverageCircuitEvaluationTime(const std::list<encrypto::motion::RunTimeStatistics>& runtimes) {
    double avg = 0.0;
    for (auto rt : runtimes) {
        auto tpp = rt.Get(encrypto::motion::RunTimeStatistics::StatisticsId::kEvaluate);
        std::chrono::duration<double, std::milli> time_ms = tpp.second - tpp.first;
        avg += time_ms.count();
    }
    return avg / runtimes.size();
}

template <encrypto::motion::MpcProtocol P>
BenchmarkOutput execBenchmark(const std::string& circuit_name,
                              size_t input_size_0,  // party 0
                              size_t input_size_1,  // party 1
                              boost::program_options::variables_map& user_options,
                              const std::string& vectorizedPath) {
    namespace mo = encrypto::motion;
    namespace be = fuse::benchmarks;

    // read FUSE IR from file
    fuse::core::CircuitContext unoptimized_circ;
    unoptimized_circ.readCircuitFromFile(be::kPathToFuseIr + circuit_name + be::kCircId);
    fuse::core::CircuitContext optimized_circ;
    optimized_circ.readCircuitFromFile(vectorizedPath + circuit_name + be::kCircId);

    // create input values for both parties together
    std::vector<bool> input_values;
    for (int i = 0; i < input_size_0 + input_size_1; ++i) {
        input_values.push_back(i % 2 == 0 ? false : true);
    }

    // create first set of parties for unvectorized execution and share input values
    encrypto::motion::PartyPointer dummyParty{CreateParty(user_options)};
    std::array<std::vector<mo::ShareWrapper>, number_of_parties> dummy_input_shares;

    for (std::size_t input_owner : {0, 1}) {
        // reserve enough space inside vector for the shares
        size_t inputSize = input_owner == 0 ? input_size_0 : input_size_1;
        dummy_input_shares[input_owner].resize(inputSize);
        for (size_t element = 0; element < inputSize; ++element) {
            // calculate share from input value and save to input shares for current party
            bool inputValue = input_values[element];
            auto input_share = dummyParty->template In<P>(inputValue, input_owner);
            dummy_input_shares[input_owner][element] = input_share;
        }
    }
    // run MPC on unvectorized circuit
    fuse::backend::evaluate(unoptimized_circ.getCircuitBufferWrapper(), dummyParty);
    dummyParty->Run();
    dummyParty->Finish();

    auto beforeStats = dummyParty->GetBackend()->GetRunTimeStatistics().front();
    auto beforeTpp = beforeStats.Get(encrypto::motion::RunTimeStatistics::StatisticsId::kEvaluate);
    std::chrono::duration<double, std::milli> beforeTime = beforeTpp.second - beforeTpp.first;

    // CommunicationOutput res;
    // assert(!dummyParty->GetCommunicationLayer().GetTransportStatistics().empty());
    // auto beforeComms = dummyParty->GetCommunicationLayer().GetTransportStatistics().front();
    // res.unoptBytes = beforeComms.number_of_bytes_sent;
    // res.unoptMsg = beforeComms.number_of_messages_sent;

    dummyParty.reset(nullptr);

    // create second set of parties for vectorized execution and share input values
    encrypto::motion::PartyPointer execParty{CreateParty(user_options)};
    std::array<std::vector<mo::ShareWrapper>, number_of_parties> exec_input_shares;
    for (std::size_t input_owner : {0, 1}) {
        // reserve enough space inside vector for the shares
        size_t inputSize = input_owner == 0 ? input_size_0 : input_size_1;
        exec_input_shares[input_owner].resize(inputSize);
        for (size_t element = 0; element < inputSize; ++element) {
            // calculate share from input value and save to input shares for current party
            bool inputValue = input_values[element];
            auto input_share = execParty->template In<P>(inputValue, input_owner);
            exec_input_shares[input_owner][element] = input_share;
        }
    }

    fuse::backend::evaluate(optimized_circ.getCircuitBufferWrapper(), execParty);
    execParty->Run();
    execParty->Finish();
    auto afterStats = execParty->GetBackend()->GetRunTimeStatistics().front();
    auto afterTpp = afterStats.Get(encrypto::motion::RunTimeStatistics::StatisticsId::kEvaluate);
    std::chrono::duration<double, std::milli> afterTime = afterTpp.second - afterTpp.first;

    // auto afterComms = execParty->GetCommunicationLayer().GetTransportStatistics().front();
    // res.optBytes = afterComms.number_of_bytes_sent;
    // res.optMsg = afterComms.number_of_messages_sent;

    execParty.reset(nullptr);

    auto b = beforeTime.count();
    auto a = afterTime.count();
    return BenchmarkOutput(b, a);

    // return res;
}
