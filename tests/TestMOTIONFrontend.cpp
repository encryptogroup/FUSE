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
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>

// MOTION
#include "base/party.h"
#include "communication/communication_layer.h"
#include "protocols/bmr/bmr_share.h"
#include "protocols/boolean_gmw/boolean_gmw_share.h"
#include "protocols/share_wrapper.h"
#include "utility/bit_vector.h"

// FUSE
#include "BristolFrontend.h"
#include "DOTBackend.h"
#include "IOHandlers.h"
#include "IR.h"
#include "InstructionVectorization.h"
#include "MOTIONBackend.h"
#include "MOTIONFrontend.h"

namespace fuse::tests::frontend {

namespace mo = encrypto::motion;

constexpr auto boolean_protocol = mo::MpcProtocol::kBooleanGmw;
constexpr auto arithmetic_protocol = mo::MpcProtocol::kArithmeticGmw;

class MotionFrontendTest : public ::testing::TestWithParam<size_t> {
   protected:
    void SetUp() override {
        SetUpDummyParties();
        ReadInMD5Circuit();
    }

    void SetUpDummyParties() {
        // create a single party on a dummy commumnication layer
        // auto comm_layers = mo::communication::MakeDummyCommunicationLayers(1);
        // dummyParty = std::make_unique<mo::Party>(std::move(comm_layers.at(0)));
        // create locally connected MOTION parties
        dummyParties = mo::MakeLocallyConnectedParties(number_of_parties /*number of parties*/,
                                                       17777u /*TCP port offset*/,
                                                       true /*logging enabled*/);

        // configure each MOTION party
        for (auto& party : dummyParties) {
            party->GetLogger()->SetEnabled(true /* detailed logging enabled */);
            party->GetConfiguration()->SetOnlineAfterSetup(false);
        }
    }

    void TearDownDummyParties() {
        for (auto& p : dummyParties) {
            p.reset(nullptr);
        }
    }

    void SetUpExecParties() {
        // create locally connected MOTION parties
        execParties = mo::MakeLocallyConnectedParties(number_of_parties /*number of parties*/,
                                                      17777u /*TCP port offset*/,
                                                      true /*logging enabled*/);

        // configure each MOTION party
        for (auto& party : execParties) {
            party->GetLogger()->SetEnabled(true /* detailed logging enabled */);
            party->GetConfiguration()->SetOnlineAfterSetup(false);
        }
    }

    void ReadInMD5Circuit() {
        md5_context = fuse::frontend::loadFUSEFromBristol("../../examples/bristol_circuits/md5.bristol");
        add32_context = fuse::frontend::loadFUSEFromBristol("../../examples/bristol_circuits/adder_32bit.bristol");
    }

    void CreateInputValues(const std::string& numberToRepresent) {
        ASSERT_EQ(numberToRepresent.size(), md5_input_size * number_of_parties);
        size_t currentCharPos = 0;
        for (char binaryChar : numberToRepresent) {
            // use integer division to select the correct party for the current character
            int receivingParty = currentCharPos / md5_input_size;
            if (binaryChar == '0') {
                md5_input_values[receivingParty].push_back(false);
            } else {
                md5_input_values[receivingParty].push_back(true);
            }
            ++currentCharPos;
        }
    }

    void CreateAdd32InputValues(const std::string& numberToRepresent) {
        ASSERT_EQ(numberToRepresent.size(), add32_input_size * number_of_parties);
        size_t currentCharPos = 0;
        for (char binaryChar : numberToRepresent) {
            // use integer division to select the correct party for the current character
            int receivingParty = currentCharPos / add32_input_size;
            if (binaryChar == '0') {
                add32_input_values[receivingParty].push_back(false);
            } else {
                add32_input_values[receivingParty].push_back(true);
            }
            ++currentCharPos;
        }
    }

    void ShareDummyAdd32InputValues() {
        for (std::size_t input_owner : {0, 1}) {
            for (std::size_t current_party : {0, 1}) {
                // reserve enough space inside vector for the shares
                dummy_add32_input_shares[current_party][input_owner].resize(add32_input_size);
                for (size_t element = 0; element < add32_input_size; ++element) {
                    // calculate share from input value and save to input shares for current party
                    bool inputValue = add32_input_values[input_owner][element];
                    auto input_share = dummyParties.at(current_party)->template In<boolean_protocol>(inputValue, input_owner);
                    dummy_add32_input_shares[current_party][input_owner][element] = input_share;
                }
            }
        }
    }

    void ShareExecAdd32InputValues() {
        for (std::size_t input_owner : {0, 1}) {
            for (std::size_t current_party : {0, 1}) {
                // reserve enough space inside vector for the shares
                add32_input_shares[current_party][input_owner].resize(add32_input_size);
                for (size_t element = 0; element < add32_input_size; ++element) {
                    // calculate share from input value and save to input shares for current party
                    bool inputValue = add32_input_values[input_owner][element];
                    auto input_share = execParties.at(current_party)->template In<boolean_protocol>(inputValue, input_owner);
                    add32_input_shares[current_party][input_owner][element] = input_share;
                }
            }
        }
    }

    void ShareDummyInputValues() {
        for (std::size_t input_owner : {0, 1}) {
            for (std::size_t current_party : {0, 1}) {
                // reserve enough space inside vector for the shares
                dummy_md5_input_shares[current_party][input_owner].resize(md5_input_size);
                for (size_t element = 0; element < md5_input_size; ++element) {
                    // calculate share from input value and save to input shares for current party
                    bool inputValue = md5_input_values[input_owner][element];
                    auto input_share = dummyParties.at(current_party)->template In<boolean_protocol>(inputValue, input_owner);
                    dummy_md5_input_shares[current_party][input_owner][element] = input_share;
                }
            }
        }
    }

    void ShareExecInputValues() {
        for (std::size_t input_owner : {0, 1}) {
            for (std::size_t current_party : {0, 1}) {
                // reserve enough space inside vector for the shares
                md5_input_shares[current_party][input_owner].resize(md5_input_size);
                for (size_t element = 0; element < md5_input_size; ++element) {
                    // calculate share from input value and save to input shares for current party
                    bool inputValue = md5_input_values[input_owner][element];
                    auto input_share = execParties.at(current_party)->template In<boolean_protocol>(inputValue, input_owner);
                    md5_input_shares[current_party][input_owner][element] = input_share;
                }
            }
        }
    }

    // input bits per party
    static constexpr size_t md5_input_size{256};
    static constexpr size_t add32_input_size{32};
    static constexpr size_t number_of_parties{2};
    std::vector<mo::PartyPointer> execParties;
    std::vector<mo::PartyPointer> dummyParties;
    // for each party the plaintext input values
    std::array<std::vector<bool>, number_of_parties> md5_input_values;
    std::array<std::vector<bool>, number_of_parties> add32_input_values;
    // for each party, the shares corresponding to the different owners of the share
    // party_id 0 : | [input share vector for inputs from party 0], [input share vector for inputs from party 1] | <- md5_input_shares[0]
    // party_id 1 : | [input share vector for inputs from party 0], [input share vector for inputs from party 1] | <- md5_input_shares[1]
    std::array<std::array<std::vector<mo::ShareWrapper>, number_of_parties>, number_of_parties> md5_input_shares;
    std::array<std::array<std::vector<mo::ShareWrapper>, number_of_parties>, number_of_parties> dummy_md5_input_shares;

    std::array<std::array<std::vector<mo::ShareWrapper>, number_of_parties>, number_of_parties> add32_input_shares;
    std::array<std::array<std::vector<mo::ShareWrapper>, number_of_parties>, number_of_parties> dummy_add32_input_shares;

    fuse::core::CircuitContext md5_context;
    fuse::core::CircuitContext add32_context;

    const mo::MpcProtocol protocol = mo::MpcProtocol::kBooleanGmw;

    const std::vector<std::string> add32_plaintext_inputs{
        "0000000000000000000000000000000000000000000000000000000000000000",
        "1111111111111111111111111111111111111111111111111111111111111111",
        "1111111111111111111111111111111100000000000000000000000000000000",
        "0000000000000000000000000000000011111111111111111111111111111111",
    };

    const std::vector<std::string> plaintext_inputs{
        "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        "00000000000000010000001000000011000001000000010100000110000001110000100000001001000010100000101100001100000011010000111000001111000100000001000100010010000100110001010000010101000101100001011100011000000110010001101000011011000111000001110100011110000111110010000000100001001000100010001100100100001001010010011000100111001010000010100100101010001010110010110000101101001011100010111100110000001100010011001000110011001101000011010100110110001101110011100000111001001110100011101100111100001111010011111000111111",
        "11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111",
        "00100100001111110110101010001000100001011010001100001000110100110001001100011001100010100010111000000011011100000111001101000100101001000000100100111000001000100010100110011111001100011101000000001000001011101111101010011000111011000100111001101100100010010100010100101000001000011110011000111000110100000001001101110111101111100101010001100110110011110011010011101001000011000110110011000000101011000010100110110111110010010111110001010000110111010011111110000100110101011011010110110101010001110000100100010111"};

    const std::vector<std::string> expected_outputs{
        "10101100000111010001111100000011110100001000111010100101011011101011011101100111101010110001111110010001011101110011000101110100",
        "11001010110110010100010010010001110010011110010000000001110110010011100001011011111111000111001000011110111101010101111101100010",
        "10110100100001110001100101010110010100011001000100111110010010010100101101010101110001101011110111011111010000000101110000000001",
        "00110111000101011111010101101000111101000010001011011011011101011100110010001101011001011110000100010111011001001111111100000001"};
};

// Instantiate the test suite below with the values to access the input and expected output vectors for MD5
INSTANTIATE_TEST_SUITE_P(Frontend,
                         MotionFrontendTest,
                         /* The indices of expected and output MD5 vectors */
                         ::testing::Values(0, 1, 2, 3));

TEST_P(MotionFrontendTest, MD5) {
    using ShareVector = std::vector<mo::ShareWrapper>;
    ASSERT_TRUE(GetParam() < plaintext_inputs.size());
    // this->CreateInputValues(plaintext_inputs[GetParam()]);
    // this->ShareDummyInputValues();
    this->CreateAdd32InputValues(add32_plaintext_inputs[GetParam()]);
    this->ShareDummyAdd32InputValues();
    std::array<std::future<void>, number_of_parties> dummyFutures;
    std::array<std::future<void>, number_of_parties> execFutures;

    core::CircuitContext circ;

    // read in using dummy parties
    for (auto current_party = 0u; current_party < this->dummyParties.size(); ++current_party) {
        dummyFutures[current_party] = std::async([this, &circ, current_party]() {
            auto circuitBufferWrapper = this->add32_context.getCircuitBufferWrapper();
            auto& party = this->dummyParties.at(current_party);
            auto outputShareMap = backend::evaluate(circuitBufferWrapper, party);
            party->Run();

            {
                // check output values
                std::stringstream result;
                for (auto outputNodeId : circuitBufferWrapper.getOutputNodeIDs()) {
                    result << outputShareMap[outputNodeId].at(0).As<bool>();
                }
                // EXPECT_STREQ(result.str().c_str(), this->expected_outputs[GetParam()].c_str());
                std::cout << result.str() << std::endl;
            }

            if (current_party == 0) {
                circ = fuse::frontend::loadFUSEFromMOTION(party);
            }
            party->Finish();
        });
    }

    // get the futures to make sure the computation is happening
    for (auto& f : dummyFutures) {
        f.get();
    }

    // now delete dummy parties and set up exec parties for execution
    this->TearDownDummyParties();
    std::ofstream out("../../tests/outputs/dot_output/motion_vectorized.txt");

    out << backend::generateDotCodeFrom(circ.getCircuitBufferWrapper()) << std::endl;
    out << "\n Vectorized:\n"
        << std::endl;
    auto mutableCirc = circ.getMutableCircuitWrapper();
    fuse::passes::vectorizeInstructons(mutableCirc, core::ir::PrimitiveOperation::And);
    out << backend::generateDotCodeFrom(mutableCirc) << std::endl;

    this->SetUpExecParties();
    this->ShareExecAdd32InputValues();

    // execution with actual parties
    for (auto current_party = 0u; current_party < this->execParties.size(); ++current_party) {
        execFutures[current_party] = std::async([this, current_party, &mutableCirc]() {
            auto outputShareMap = backend::evaluate(mutableCirc, this->execParties[current_party]);
            this->execParties[current_party]->Run();
            {
                // check output values
                std::stringstream result;
                for (auto outputNodeId : mutableCirc.getOutputNodeIDs()) {
                    result << outputShareMap[outputNodeId].at(0).As<bool>();
                }
                // EXPECT_STREQ(result.str().c_str(), this->expected_outputs[GetParam()].c_str());
                std::cout << result.str() << std::endl;
            }
            // call finish for cleanup
            this->execParties[current_party]->Finish();
        });
    }
    for (auto& f : execFutures) {
        f.get();
    }
}

}  // namespace fuse::tests::frontend
