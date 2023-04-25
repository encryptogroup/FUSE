#include "common.h"

#include <BristolFrontend.h>
#include <ConstantFolder.h>
#include <DeadNodeEliminator.h>
#include <FrequentSubcircuitReplacement.h>
#include <HyCCFrontend.h>
#include <IOHandlers.h>
#include <IR.h>
#include <InstructionVectorization.h>
#include <ModuleBuilder.h>
#include <libcircuit/simple_circuit.h>


#include <bitset>
#include <filesystem>
#include <fstream>
#include <gzip/compress.hpp>
#include <numeric>
#include <regex>

namespace fuse::benchmarks {

std::string getNameFromPath(const std::string& path, bool hasFileSuffix) {
    auto const pos = path.find_last_of('/') + 1;
    if (hasFileSuffix) {
        auto const end = path.find_last_of('.');
        return path.substr(pos, end - pos);
    } else {
        return path.substr(pos);
    }
}

std::unordered_map<std::string, simple_circuitt> loadHyccFromCircFile(const std::string& pathToCmb) {
    std::vector<std::string> hyccCircuitFiles;
    std::filesystem::path cmbPath(pathToCmb);
    std::filesystem::path hyccCircuitDirectory(cmbPath.parent_path());
    std::unordered_map<std::string, simple_circuitt> res;

    loggert hyccLogger_;
    std::ifstream cmbContent(cmbPath);
    // for (const auto& entry :
    //      std::filesystem::directory_iterator(hyccCircuitDirectory)) {
    //     if (entry.exists() && !entry.path().empty() &&
    //         entry.path().extension() == ".circ") {
    //         hyccCircuitFiles.push_back(
    //             entry.path().stem().string());
    //     }
    // }
    std::string line;
    while (std::getline(cmbContent, line)) {
        hyccCircuitFiles.push_back(cmbPath.parent_path() / line);
    }

    // for every circuit file in the directory:
    for (const auto& filename : hyccCircuitFiles) {
        const auto file_path = hyccCircuitDirectory / filename;
        simple_circuitt hycc_circuit(hyccLogger_, "");

        std::ifstream file(file_path);
        hycc_circuit.read(file);
        auto circuit_name = hycc_circuit.name();
        res.emplace(circuit_name, std::move(hycc_circuit));
    }
    return res;
}

void generateFuseFromBristol() {
    namespace io = fuse::core::util::io;

    for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(kPathToBristolCircuits)) {
        // filter for the .bristol files
        if (dirEntry.exists() && !dirEntry.path().empty() && dirEntry.path().extension() == ".bristol") {
            // split path to bristol file into name and path
            auto circName = dirEntry.path().stem();
            auto circPath = dirEntry.path().string();
            std::string goalPath = kPathToFuseIr + circName.string() + kCircId;
            fuse::frontend::loadFUSEFromBristolToFile(circPath, goalPath);
        }
    }
}

/*
    [0 1 2 ... rotParam-1 | rotParam ... n]
--> [rotParam ... n       | 0 1 2 ... rotParam - 1]
*/
void genRotateLeft(fuse::frontend::CircuitBuilder& circ, const int rotationParam) {
    auto boolType = circ.addDataType(pt::Bool);
    std::array<Id, wordSize> inputs;
    // add input nodes
    for (int i = 0; i < wordSize; ++i) {
        inputs[i] = circ.addInputNode(boolType);
    }
    // set output nodes with correct wiring
    // (1) All the nodes that are shifted left
    for (int i = rotationParam; i < wordSize; ++i) {
        circ.addOutputNode(boolType, {inputs[i]});
    }
    // (2) Put rest of the nodes afterwards back in
    for (int i = 0; i < rotationParam; ++i) {
        circ.addOutputNode(boolType, {inputs[i]});
    }
}

/*
    [0 1 2 ... rotParam-1 | rotParam ... n]
--> [rotParam ... n       | 0 1 2 ... rotParam - 1]
*/
void genRotateRight(fuse::frontend::CircuitBuilder& circ, const int rotationParam) {
    auto boolType = circ.addDataType(pt::Bool);
    std::array<Id, wordSize> inputs;
    // add input nodes
    for (int i = 0; i < wordSize; ++i) {
        inputs[i] = circ.addInputNode(boolType);
    }
    // set output nodes with correct wiring
    // (1) Rotated nodes to the beginning
    for (int i = wordSize - rotationParam; i < wordSize; ++i) {
        circ.addOutputNode(boolType, {inputs[i]});
    }
    // (2) All the nodes that are shifted right
    for (int i = 0; i < rotationParam; ++i) {
        circ.addOutputNode(boolType, {inputs[i]});
    }
}

void genShiftLeft(fuse::frontend::CircuitBuilder& circ, const int shiftParam) {
    auto boolType = circ.addDataType(pt::Bool);
    std::array<Id, wordSize> inputs;
    // add input nodes
    for (int i = 0; i < wordSize; ++i) {
        inputs[i] = circ.addInputNode(boolType);
    }
    // set output nodes with correct wiring
    // (1) All the nodes that are shifted left
    for (int i = shiftParam; i < wordSize; ++i) {
        circ.addOutputNode(boolType, {inputs[i]});
    }
    // (2) Put constant 0/false for rest of the nodes afterwards back in
    Id zero = circ.addConstantNodeWithPayload(false);
    for (int i = 0; i < shiftParam; ++i) {
        circ.addOutputNode(boolType, {zero});
    }
}

void genShiftRight(fuse::frontend::CircuitBuilder& circ, const int shiftParam) {
    auto boolType = circ.addDataType(pt::Bool);
    std::array<Id, wordSize> inputs;
    // add input nodes
    for (int i = 0; i < wordSize; ++i) {
        inputs[i] = circ.addInputNode(boolType);
    }
    Id zero = circ.addConstantNodeWithPayload(false);
    // set output nodes with correct wiring
    // (1) Put constant 0/false for the out-shifted numbers
    for (int i = wordSize - shiftParam; i < wordSize; ++i) {
        circ.addOutputNode(boolType, {zero});
    }
    // (2) All the nodes that are shifted right -> zeros
    for (int i = 0; i < shiftParam; ++i) {
        circ.addOutputNode(boolType, {inputs[i]});
    }
}

void genCH(fuse::frontend::CircuitBuilder& circ) {
    auto boolType = circ.addDataType(pt::Bool);
    std::array<Id, wordSize> inputs_x;
    std::array<Id, wordSize> inputs_y;
    std::array<Id, wordSize> inputs_z;
    // add input nodes
    for (int i = 0; i < wordSize; ++i) {
        inputs_x[i] = circ.addInputNode(boolType);
    }
    for (int i = 0; i < wordSize; ++i) {
        inputs_y[i] = circ.addInputNode(boolType);
    }
    for (int i = 0; i < wordSize; ++i) {
        inputs_z[i] = circ.addInputNode(boolType);
    }
    std::array<Id, wordSize> x_and_y;
    for (int i = 0; i < wordSize; ++i) {
        x_and_y[i] = circ.addNode(op::And, {inputs_x[i], inputs_y[i]});
    }
    std::array<Id, wordSize> not_x;
    for (int i = 0; i < wordSize; ++i) {
        not_x[i] = circ.addNode(op::Not, {inputs_x[i]});
    }
    std::array<Id, wordSize> not_x_and_z;
    for (int i = 0; i < wordSize; ++i) {
        not_x_and_z[i] = circ.addNode(op::And, {not_x[i], inputs_z[i]});
    }
    std::array<Id, wordSize> res;
    for (int i = 0; i < wordSize; ++i) {
        res[i] = circ.addNode(op::Xor, {x_and_y[i], not_x_and_z[i]});
    }
    // set input nodes to result of computation
    for (int i = 0; i < wordSize; ++i) {
        circ.addOutputNode(boolType, {res[i]});
    }
}

void genMAJ(fuse::frontend::CircuitBuilder& circ) {
    auto boolType = circ.addDataType(pt::Bool);
    std::array<Id, wordSize> inputs_x;
    std::array<Id, wordSize> inputs_y;
    std::array<Id, wordSize> inputs_z;
    // add input nodes
    for (int i = 0; i < wordSize; ++i) {
        inputs_x[i] = circ.addInputNode(boolType);
    }
    for (int i = 0; i < wordSize; ++i) {
        inputs_y[i] = circ.addInputNode(boolType);
    }
    for (int i = 0; i < wordSize; ++i) {
        inputs_z[i] = circ.addInputNode(boolType);
    }

    std::array<Id, wordSize> x_and_y;
    for (int i = 0; i < wordSize; ++i) {
        x_and_y[i] = circ.addNode(op::And, {inputs_x[i], inputs_y[i]});
    }
    std::array<Id, wordSize> x_and_z;
    for (int i = 0; i < wordSize; ++i) {
        x_and_z[i] = circ.addNode(op::And, {inputs_x[i], inputs_z[i]});
    }
    std::array<Id, wordSize> y_and_z;
    for (int i = 0; i < wordSize; ++i) {
        y_and_z[i] = circ.addNode(op::And, {inputs_y[i], inputs_z[i]});
    }
    std::array<Id, wordSize> res_xor;
    for (int i = 0; i < wordSize; ++i) {
        res_xor[i] = circ.addNode(op::Xor, {x_and_y[i], x_and_z[i], y_and_z[i]});
    }
    for (int i = 0; i < wordSize; ++i) {
        circ.addOutputNode(boolType, {res_xor[i]});
    }
}

void genEP0(fuse::frontend::CircuitBuilder& circ) {
    auto boolType = circ.addDataType(pt::Bool);
    std::vector<Id> inputs_x;
    inputs_x.resize(wordSize);
    // add input nodes
    for (int i = 0; i < wordSize; ++i) {
        inputs_x[i] = circ.addInputNode(boolType);
    }
    auto rotRight2 = circ.addCallToSubcircuitNode(inputs_x, "ROTRIGHT_2");
    auto rotRight13 = circ.addCallToSubcircuitNode(inputs_x, "ROTRIGHT_13");
    auto rotRight22 = circ.addCallToSubcircuitNode(inputs_x, "ROTRIGHT_22");
    // bitwise xor of all call results
    std::array<Id, wordSize> res;
    for (unsigned i = 0; i < wordSize; ++i) {
        res[i] = circ.addNode(op::Xor, {rotRight2, rotRight13, rotRight22}, {i, i, i});
    }
    for (unsigned i = 0; i < wordSize; ++i) {
        circ.addOutputNode(boolType, {res[i]});
    }
}

void genEP1(fuse::frontend::CircuitBuilder& circ) {
    auto boolType = circ.addDataType(pt::Bool);
    std::vector<Id> inputs_x;
    inputs_x.resize(wordSize);
    // add input nodes
    for (int i = 0; i < wordSize; ++i) {
        inputs_x[i] = circ.addInputNode(boolType);
    }
    auto rotRight6 = circ.addCallToSubcircuitNode(inputs_x, "ROTRIGHT_6");
    auto rotRight11 = circ.addCallToSubcircuitNode(inputs_x, "ROTRIGHT_11");
    auto rotRight25 = circ.addCallToSubcircuitNode(inputs_x, "ROTRIGHT_25");
    // bitwise xor of all call results
    std::array<Id, wordSize> res;
    for (unsigned i = 0; i < wordSize; ++i) {
        res[i] = circ.addNode(op::Xor, {rotRight6, rotRight11, rotRight25}, {i, i, i});
    }
    for (unsigned i = 0; i < wordSize; ++i) {
        circ.addOutputNode(boolType, {res[i]});
    }
}

void genSIG0(fuse::frontend::CircuitBuilder& circ) {
    auto boolType = circ.addDataType(pt::Bool);
    // add input nodes: 32 bits
    std::vector<Id> inputs_x;
    inputs_x.resize(wordSize);
    for (int i = 0; i < wordSize; ++i) {
        inputs_x[i] = circ.addInputNode(boolType);
    }
    auto rotRight18 = circ.addCallToSubcircuitNode(inputs_x, "ROTRIGHT_18");
    auto rotRight7 = circ.addCallToSubcircuitNode(inputs_x, "ROTRIGHT_7");
    auto rShift3 = circ.addCallToSubcircuitNode(inputs_x, "RSHIFT_3");

    std::array<Id, wordSize> res;
    for (unsigned i = 0; i < wordSize; ++i) {
        res[i] = circ.addNode(op::Xor, {rotRight18, rotRight7, rShift3}, {i, i, i});
    }
    for (unsigned i = 0; i < wordSize; ++i) {
        circ.addOutputNode(boolType, {res[i]});
    }
}

void genSIG1(fuse::frontend::CircuitBuilder& circ) {
    auto boolType = circ.addDataType(pt::Bool);
    std::vector<Id> inputs_x;
    inputs_x.resize(wordSize);
    // add input nodes
    for (int i = 0; i < wordSize; ++i) {
        inputs_x[i] = circ.addInputNode(boolType);
    }
    auto rotRight17 = circ.addCallToSubcircuitNode(inputs_x, "ROTRIGHT_17");
    auto rotRight19 = circ.addCallToSubcircuitNode(inputs_x, "ROTRIGHT_19");
    auto rShift10 = circ.addCallToSubcircuitNode(inputs_x, "RSHIFT_10");
    // bitwise xor of all call results
    std::array<Id, wordSize> res;
    for (unsigned i = 0; i < wordSize; ++i) {
        res[i] = circ.addNode(op::Xor, {rotRight17, rotRight19, rShift10}, {i, i, i});
    }
    for (unsigned i = 0; i < wordSize; ++i) {
        circ.addOutputNode(boolType, {res[i]});
    }
}

void generateCallbacks(fuse::frontend::ModuleBuilder& mod) {
    // generate all rotation circuits
    auto rotRight2 = mod.addCircuit("ROTRIGHT_2");
    genRotateRight(*rotRight2, 2);
    auto rotRight13 = mod.addCircuit("ROTRIGHT_13");
    genRotateRight(*rotRight13, 13);
    auto rotRight22 = mod.addCircuit("ROTRIGHT_22");
    genRotateRight(*rotRight22, 22);
    auto rotRight6 = mod.addCircuit("ROTRIGHT_6");
    genRotateRight(*rotRight6, 6);
    auto rotRight11 = mod.addCircuit("ROTRIGHT_11");
    genRotateRight(*rotRight11, 11);
    auto rotRight25 = mod.addCircuit("ROTRIGHT_25");
    genRotateRight(*rotRight25, 25);
    auto rotRight7 = mod.addCircuit("ROTRIGHT_7");
    genRotateRight(*rotRight7, 7);
    auto rotRight18 = mod.addCircuit("ROTRIGHT_18");
    genRotateRight(*rotRight18, 18);
    auto rotRight3 = mod.addCircuit("ROTRIGHT_3");
    genRotateRight(*rotRight3, 3);
    auto rotRight17 = mod.addCircuit("ROTRIGHT_17");
    genRotateRight(*rotRight17, 17);
    auto rotRight19 = mod.addCircuit("ROTRIGHT_19");
    genRotateRight(*rotRight19, 19);
    auto rotRight10 = mod.addCircuit("ROTRIGHT_10");
    genRotateRight(*rotRight10, 10);

    // generate right shifts circuits and sig, ep, maj, ch
    auto rshift3 = mod.addCircuit("RSHIFT_3");
    genShiftRight(*rshift3, 3);
    auto rshift10 = mod.addCircuit("RSHIFT_10");
    genShiftRight(*rshift10, 10);
    auto ch = mod.addCircuit("CH");
    genCH(*ch);
    auto maj = mod.addCircuit("MAJ");
    genCH(*maj);
    auto EP0 = mod.addCircuit("EP0");
    genCH(*EP0);
    auto EP1 = mod.addCircuit("EP1");
    genCH(*EP1);
    auto SIG0 = mod.addCircuit("SIG0");
    genCH(*SIG0);
    auto SIG1 = mod.addCircuit("SIG1");
    genCH(*SIG1);
}

void generateShaInit(fuse::frontend::CircuitBuilder& circ) {
    std::array<std::bitset<32>, 8> init_states;
    init_states[0] = 0x6a09e667;
    init_states[1] = 0xbb67ae85;
    init_states[2] = 0x3c6ef372;
    init_states[3] = 0xa54ff53a;
    init_states[4] = 0x510e527f;
    init_states[5] = 0x9b05688c;
    init_states[6] = 0x1f83d9ab;
    init_states[7] = 0x5be0cd19;
    auto boolType = circ.addDataType(pt::Bool);
    auto falseVal = circ.addConstantNodeWithPayload(false);
    auto trueVal = circ.addConstantNodeWithPayload(true);
    for (auto state : init_states) {
        auto bitString = state.to_string();
        for (auto bit : bitString) {
            if (bit = '0') {
                circ.addOutputNode(boolType, {falseVal});
            }
            if (bit = '1') {
                circ.addOutputNode(boolType, {trueVal});
            } else {
                std::runtime_error("Unexpected value for bitstring in sha init: " + bit);
            }
        }
    }
}

void generateShaTransform(fuse::frontend::CircuitBuilder& circ) {
    std::array<std::bitset<32>, 64> k_vals{
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

    auto boolType = circ.addDataType(pt::Bool);
    auto falseVal = circ.addConstantNodeWithPayload(false);
    auto trueVal = circ.addConstantNodeWithPayload(true);

    // input: 256 bits current state and 512 bits data
    std::array<Id, 256> state_input;
    std::array<Id, 512> data_input;
    for (int i = 0; i < state_input.size(); ++i) {
        state_input[i] = circ.addInputNode(boolType);
    }
    for (int i = 0; i < data_input.size(); ++i) {
        data_input[i] = circ.addInputNode(boolType);
    }
    /*
    for (i,j = 0; i < 16; ++i, j+=4)
        m[i] = data[j] | data[j+1] | data[j+2] | data[j+3]
    for (i = 16; i < 64; ++i)
        m[i] = SIG1(m[i-2]) + m[i-7] + SIG0(m[i-15]) + m[m-16]
    */
    std::array<Id, 2048> m;
    // initialize first rows of m with input data
    for (int i = 0; i < data_input.size(); ++i) {
        m[i] = data_input[i];
    }
    // calculate rest of values for m with SIG0 and SIG1
    for (int i = 16; i < 64; ++i) {
        int m_i = i * wordSize;
        int m_i_minus_2 = (i - 2) * wordSize;
        int m_i_minus_7 = (i - 7) * wordSize;
        int m_i_minus_15 = (i - 15) * wordSize;
        int m_i_minus_16 = (i - 16) * wordSize;

        // calculate sub_expressions: SIG0 and SIG1 call
        std::vector<Id> sig1_call_params;
        std::vector<Id> sig0_call_params;
        sig1_call_params.resize(wordSize);
        for (int j = 0; j < wordSize; ++j) {
            sig1_call_params[j] = m[m_i_minus_2 + j];
        }
        sig0_call_params.resize(wordSize);
        for (int j = 0; j < wordSize; ++j) {
            sig0_call_params[j] = m[m_i_minus_15 + j];
        }
        auto sig1_call = circ.addCallToSubcircuitNode(sig1_call_params, "SIG1");
        auto sig0_call = circ.addCallToSubcircuitNode(sig0_call_params, "SIG0");

        // Merge node outputs to use integer addition
        // initialize ids with the same caller id
        std::vector<Id> sig1_merge_input_ids(wordSize, sig1_call);
        std::vector<Offset> sig1_merge_input_offsets(wordSize);
        // initalize offsets with increasing numbers starting from 0 ... 31
        std::iota(sig1_merge_input_offsets.begin(), sig1_merge_input_offsets.end(), 0);
        auto sig1_int = circ.addNode(op::Merge, sig1_merge_input_ids, sig1_merge_input_offsets);

        std::vector<Id> sig0_merge_ids(wordSize, sig0_call);
        std::vector<Offset> sig0_merge_offsets(wordSize);
        std::iota(sig0_merge_offsets.begin(), sig0_merge_offsets.end(), 0);
        auto sig0_int = circ.addNode(op::Merge, sig0_merge_ids, sig0_merge_offsets);

        std::vector<Id> m_i_minus_7_ids(wordSize);
        std::vector<Id> m_i_minus_16_ids(wordSize);
        std::iota(m_i_minus_7_ids.begin(), m_i_minus_7_ids.end(), m_i_minus_7);
        std::iota(m_i_minus_16_ids.begin(), m_i_minus_16_ids.end(), m_i_minus_16);
        auto m_i_minus_7_int = circ.addNode(op::Merge, m_i_minus_7_ids);
        auto m_i_minus_16_int = circ.addNode(op::Merge, m_i_minus_16_ids);

        // perform addition on integerized values
        Id subExpr = circ.addNode(op::Add, {sig1_int, sig0_int, m_i_minus_7_int, m_i_minus_16_int});
        Id subExprToBools = circ.addSplitNode(type::UInt32, subExpr);
        // write computed sub expression to m array
        for (int j = 0; j < wordSize; ++j) {
            m[m_i + j] = circ.addSelectOffsetNode(subExprToBools, j);
        }
    }

    // initialize all temporaries with node IDs
    std::vector<Id> a, b, c, d, e, f, g, h, t1, t2, k_i, m_i;
    a.reserve(32);
    b.reserve(32);
    c.reserve(32);
    d.reserve(32);
    e.reserve(32);
    f.reserve(32);
    g.reserve(32);
    h.reserve(32);
    t1.reserve(32);
    t2.reserve(32);
    k_i.reserve(32);
    m_i.reserve(32);

    for (int i = 0; i < wordSize; ++i) {
        a[i] = state_input[i];
        b[i] = state_input[i + wordSize];
        c[i] = state_input[i + (wordSize * 2)];
        d[i] = state_input[i + (wordSize * 3)];
        e[i] = state_input[i + (wordSize * 4)];
        f[i] = state_input[i + (wordSize * 5)];
        g[i] = state_input[i + (wordSize * 6)];
        h[i] = state_input[i + (wordSize * 7)];
    }

    std::vector<Offset> callOffsets;
    callOffsets.reserve(32);
    std::iota(callOffsets.begin(), callOffsets.end(), 0);

    // compute
    for (int i = 0; i < 64; ++i) {
        // (1) compute subexpressions in t1 = h + EP1(e) + CH(e,f,g) + k[i] + m[i];
        Id call_ep1 = circ.addCallToSubcircuitNode(e, "EP1");
        std::vector<Id> call_ch_params;
        call_ch_params.insert(call_ch_params.end(), e.begin(), e.end());
        call_ch_params.insert(call_ch_params.end(), f.begin(), f.end());
        call_ch_params.insert(call_ch_params.end(), g.begin(), g.end());
        Id call_ch = circ.addCallToSubcircuitNode(call_ch_params, "CH");
        auto k_i_string = k_vals[i].to_string();
        for (int j = 0; j < wordSize; ++j) {
            m_i[j] = m[j + (wordSize * i)];
            k_i[j] = k_i_string.at(j) == '1' ? trueVal : falseVal;
        }
        // merge boolean outputs to an integer
        Id h_int = circ.addNode(op::Merge, h);
        std::vector<Id> call_ep1_ids(32, call_ep1);
        std::vector<Id> call_ch_ids(32, call_ch);
        Id call_ep1_int = circ.addNode(op::Merge, call_ep1_ids, callOffsets);
        Id call_ch_int = circ.addNode(op::Merge, call_ch_ids, callOffsets);
        Id k_i_int = circ.addNode(op::Merge, k_i);
        Id m_i_int = circ.addNode(op::Merge, m_i);
        // compute addition and split output again to update t1 with SelOffset nodes
        Id t1_int = circ.addNode(op::Add, {h_int, call_ep1_int, call_ch_int, k_i_int, m_i_int});
        Id t1_split = circ.addSplitNode(type::UInt32, t1_int);
        for (int j = 0; j < wordSize; ++j) {
            t1[j] = circ.addSelectOffsetNode(t1_split, j);
        }

        // (2) compute subexpressions in t2 = EP0(a) + MAJ(a,b,c);
        Id call_ep0 = circ.addCallToSubcircuitNode(a, "EP0");
        std::vector<Id> call_maj_params;
        call_maj_params.insert(call_maj_params.end(), a.begin(), a.end());
        call_maj_params.insert(call_maj_params.end(), b.begin(), b.end());
        call_maj_params.insert(call_maj_params.end(), c.begin(), c.end());
        Id call_maj = circ.addCallToSubcircuitNode(call_maj_params, "MAJ");
        // merge boolean outputs to integer
        std::vector<Id> call_ep0_ids(32, call_ep0);
        std::vector<Id> call_maj_ids(32, call_maj);
        Id call_ep0_int = circ.addNode(op::Merge, call_ep0_ids, callOffsets);
        Id call_maj_int = circ.addNode(op::Merge, call_maj_ids, callOffsets);
        // compute addition and split outputs again to update contents of t2
        Id t2_int = circ.addNode(op::Add, {call_ep0_int, call_maj_int});
        Id t2_split = circ.addSplitNode(type::UInt32, t2_int);
        for (int j = 0; j < wordSize; ++j) {
            t2[j] = circ.addSelectOffsetNode(t2_split, j);
        }

        // (3) update h, g, f
        h = g;
        g = f;
        f = e;

        // (4) compute subexpressions in e = d + t1 -> merge boolean outputs and compute Addition
        Id d_int = circ.addNode(op::Merge, d);
        Id e_int = circ.addNode(op::Add, {d_int, t2_int});
        // split outputs to update contents of e
        Id e_split = circ.addSplitNode(type::UInt32, e_int);
        for (int j = 0; j < wordSize; ++j) {
            e[j] = circ.addSelectOffsetNode(e_split, j);
        }

        // update d, c, b
        d = c;
        c = b;
        b = a;

        // compute a = t1 + t2, split outputs and update contens of a
        Id a_int = circ.addNode(op::Add, {t1_int, t2_int});
        Id a_split = circ.addSplitNode(type::UInt32, a_int);
        for (int j = 0; j < wordSize; ++j) {
            a[j] = circ.addSelectOffsetNode(a_split, j);
        }
    }

    // output: 256 bits transformed/updated state
    std::vector<Id> state_0, state_1, state_2, state_3, state_4, state_5, state_6, state_7;
    state_0.reserve(wordSize);
    state_1.reserve(wordSize);
    state_2.reserve(wordSize);
    state_3.reserve(wordSize);
    state_4.reserve(wordSize);
    state_5.reserve(wordSize);
    state_6.reserve(wordSize);
    state_7.reserve(wordSize);
    for (int i = 0; i < wordSize; ++i) {
        state_0[i] = state_input[i];
        state_1[i] = state_input[i + wordSize];
        state_2[i] = state_input[i + (wordSize * 2)];
        state_3[i] = state_input[i + (wordSize * 3)];
        state_4[i] = state_input[i + (wordSize * 4)];
        state_5[i] = state_input[i + (wordSize * 5)];
        state_6[i] = state_input[i + (wordSize * 6)];
        state_7[i] = state_input[i + (wordSize * 7)];
    }
    // compute new states and set them as outputs
    {
        Id state_0_int = circ.addNode(op::Merge, state_0);
        Id state_1_int = circ.addNode(op::Merge, state_1);
        Id state_2_int = circ.addNode(op::Merge, state_2);
        Id state_3_int = circ.addNode(op::Merge, state_3);
        Id state_4_int = circ.addNode(op::Merge, state_4);
        Id state_5_int = circ.addNode(op::Merge, state_5);
        Id state_6_int = circ.addNode(op::Merge, state_6);
        Id state_7_int = circ.addNode(op::Merge, state_7);
        Id final_a_int = circ.addNode(op::Merge, a);
        Id final_b_int = circ.addNode(op::Merge, b);
        Id final_c_int = circ.addNode(op::Merge, c);
        Id final_d_int = circ.addNode(op::Merge, d);
        Id final_e_int = circ.addNode(op::Merge, e);
        Id final_f_int = circ.addNode(op::Merge, f);
        Id final_g_int = circ.addNode(op::Merge, g);
        Id final_h_int = circ.addNode(op::Merge, h);
        Id new_state_0_int = circ.addNode(op::Add, {state_0_int, final_a_int});
        Id new_state_1_int = circ.addNode(op::Add, {state_1_int, final_b_int});
        Id new_state_2_int = circ.addNode(op::Add, {state_2_int, final_c_int});
        Id new_state_3_int = circ.addNode(op::Add, {state_3_int, final_d_int});
        Id new_state_4_int = circ.addNode(op::Add, {state_4_int, final_e_int});
        Id new_state_5_int = circ.addNode(op::Add, {state_5_int, final_f_int});
        Id new_state_6_int = circ.addNode(op::Add, {state_6_int, final_g_int});
        Id new_state_7_int = circ.addNode(op::Add, {state_7_int, final_h_int});
        Id new_state_0_split = circ.addSplitNode(type::UInt32, new_state_0_int);
        Id new_state_1_split = circ.addSplitNode(type::UInt32, new_state_1_int);
        Id new_state_2_split = circ.addSplitNode(type::UInt32, new_state_2_int);
        Id new_state_3_split = circ.addSplitNode(type::UInt32, new_state_3_int);
        Id new_state_4_split = circ.addSplitNode(type::UInt32, new_state_4_int);
        Id new_state_5_split = circ.addSplitNode(type::UInt32, new_state_5_int);
        Id new_state_6_split = circ.addSplitNode(type::UInt32, new_state_6_int);
        Id new_state_7_split = circ.addSplitNode(type::UInt32, new_state_7_int);
        std::vector<size_t> boolOutputType;
        for (Offset i = 0; i < wordSize; ++i) circ.addOutputNode(boolOutputType, {new_state_0_split}, {i});
        for (Offset i = 0; i < wordSize; ++i) circ.addOutputNode(boolOutputType, {new_state_1_split}, {i});
        for (Offset i = 0; i < wordSize; ++i) circ.addOutputNode(boolOutputType, {new_state_2_split}, {i});
        for (Offset i = 0; i < wordSize; ++i) circ.addOutputNode(boolOutputType, {new_state_3_split}, {i});
        for (Offset i = 0; i < wordSize; ++i) circ.addOutputNode(boolOutputType, {new_state_4_split}, {i});
        for (Offset i = 0; i < wordSize; ++i) circ.addOutputNode(boolOutputType, {new_state_5_split}, {i});
        for (Offset i = 0; i < wordSize; ++i) circ.addOutputNode(boolOutputType, {new_state_6_split}, {i});
        for (Offset i = 0; i < wordSize; ++i) circ.addOutputNode(boolOutputType, {new_state_7_split}, {i});
    }
}

void generateShaReverseBytes(fuse::frontend::CircuitBuilder& circ) {
    // 256 bits input bits
    auto boolType = circ.addDataType(pt::Bool);
    std::array<Id, 256> inputs;
    for (int i = 0; i < 256; ++i) {
        inputs[i] = circ.addInputNode(boolType);
    }
    // read bytes from the back and then copy bit-wise the input nodes to output nodes
    for (int i = (inputs.size() / 8) - 1; i >= 0; --i) {
        for (int j = 0; j < 8; ++j) {
            circ.addOutputNode(boolType, {inputs[(8 * i) + j]});
        }
    }
    // 256 output bits with reverse byte endianness
}

void generateShaMain(fuse::frontend::CircuitBuilder& circ) {
    // 512 bits input data
    auto boolType = circ.addDataType(pt::Bool);
    std::vector<Id> inputs(512);
    for (int i = 0; i < 512; ++i) {
        inputs[i] = circ.addInputNode(boolType);
    }

    // call sha init for initial state
    Id shaInit = circ.addCallToSubcircuitNode({}, "SHA256_INIT");
    std::vector<Id> shaInitVector(256);
    for (int i = 0; i < 256; ++i) {
        shaInitVector[i] = circ.addSelectOffsetNode(shaInit, i);
    }

    // call transform with state, data for new state
    std::vector<Id> state_date_param_transform(256 + 512);
    state_date_param_transform.insert(state_date_param_transform.end(), shaInitVector.begin(), shaInitVector.end());
    state_date_param_transform.insert(state_date_param_transform.end(), inputs.begin(), inputs.end());
    Id callTransform = circ.addCallToSubcircuitNode(state_date_param_transform, "SHA256_TRANSFORM");
    std::vector<Id> transformVec(256);
    for (int i = 0; i < 256; ++i) {
        transformVec[i] = circ.addSelectOffsetNode(callTransform, i);
    }

    // call reverse bytes for big-endian state result
    Id reverse = circ.addCallToSubcircuitNode(transformVec, "SHA256_REVERSE");
    std::vector<size_t> boolOutputType{boolType};
    for (unsigned i = 0; i < 256; ++i) {
        circ.addOutputNode(boolOutputType, {reverse}, {i});
    }
}

void generateSHA256() {
    // BYTE - UInt8
    // WORD - UInt32
    // SHA256_BLOCK_SIZE - 32
    /*
        SHA256_CTX:
        - uint32_t  datalen
        - uint64_t  bitlen
        - uint8_t   data[64]  (512 bits)
        - uint32_t  state[8]  (256 bits)
        --> uint8_t hash [32] (256 bits)
    */

    /*
    Steps:
    1) sha256 init      - define initial state
    2) sha256 update    - for all 64 bit data blocks: call sha256_transform on ctx + data
    3) sha256 transform:    - do some operations to calculate m[] (2048 bits)
                            - do a LOT of shuffling (put content of for loop in function and call function)
                            - return new state
    4) sha256 final
    */

    // generate all necessary SHIFT and ROTATERIGHT operations!
    fuse::frontend::ModuleBuilder mod;
    generateCallbacks(mod);

    auto sha256_init = mod.addCircuit("SHA256_INIT");
    generateShaInit(*sha256_init);

    auto sha256_transform = mod.addCircuit("SHA256_TRANSFORM");
    generateShaTransform(*sha256_transform);

    auto sha256_reverse = mod.addCircuit("SHA256_REVERSE");
    generateShaReverseBytes(*sha256_reverse);

    auto sha256_main = mod.addCircuit("SHA256_MAIN");
    generateShaMain(*sha256_main);

    mod.setEntryCircuitName("SHA256_MAIN");
    fuse::core::ModuleContext context(mod);
    context.writeModuleToFile(kPathToFuseIr + "OWN_SHA256" + kModId);
}

void generateFuseFromHycc() {
    for (const auto& hyccExample : kHyccCircuits) {
        std::filesystem::path cmb(std::get<0>(hyccExample));
        auto name = cmb.parent_path().stem().string();
        std::string fusePath = kPathToFuseIr + name + kModId;
        fuse::frontend::loadFUSEFromHyCCAndSaveToFile(std::get<0>(hyccExample), fusePath, std::get<1>(hyccExample));
    }
}

void zipCircs(const std::string& sourcePath, const std::string& goalPath, bool binary = true) {
    for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(sourcePath)) {
        if (dirEntry.exists() && !dirEntry.path().empty()) {
            std::string circName = "";
            std::string compressedData;
            if (binary) {
                circName = dirEntry.path().filename().string();
                auto data = fuse::core::util::io::readFlatBufferFromBinary(dirEntry.path().string());
                compressedData = gzip::compress(data.data(), data.size());
            } else {
                circName = dirEntry.path().stem().string();
                auto data = fuse::core::util::io::readTextFile(dirEntry.path().string());
                compressedData = gzip::compress(data.data(), data.size());
            }
            fuse::core::util::io::writeCompressedStringToBinaryFile(goalPath + circName + ".z", compressedData);
        }
    }
}

fuse::core::ModuleContext fsrOnFuseIr(fuse::core::CircuitContext& circ) {
    auto mutableCirc = circ.getMutableCircuitWrapper();
    constexpr int timeoutSeconds = 60 * 5;  // 5 mins
    constexpr int try_modes = 1;
    auto modContext = fuse::passes::automaticallyReplaceFrequentSubcircuits(circ, try_modes, timeoutSeconds);
    // auto modContext = fuse::passes::replaceFrequentSubcircuits(circ, 28, 0);
    return modContext;
}

void fsr() {
    std::fstream out(kOutputPath + "fsr_sizes_log.txt", std::ios::in | std::ios::out | std::ios::app);
    out.seekg(0, std::ios::end);
    if (out.tellg() == 0) {
        // file is emtpy -> add first line
        out << "circuit, size_before, number_of_nodes_before_fsr, size_after, number_of_nodes_after_fsr" << std::endl;
    }

    for (const auto& name : kToOptimize) {
        fuse::core::CircuitContext cont;
        cont.readCircuitFromFile(kPathToFuseIr + name + kCircId);

        // for outputs
        size_t numNodesBefore = cont.getReadOnlyCircuit()->getNumberOfNodes();
        fuse::frontend::ModuleBuilder mb;
        mb.addSerializedCircuit(cont.getBufferPointer(), cont.getBufferSize());
        mb.setEntryCircuitName(name);
        mb.finishAndWriteToFile(kPathToFsrFuseIr + name + "_unoptimized" + kModId);
        size_t sizeBefore = std::filesystem::file_size(kPathToFsrFuseIr + name + "_unoptimized" + kModId);
        out << name << kSep
            << sizeBefore << kSep
            << numNodesBefore << kSep;
        out.flush();

        auto mod = fsrOnFuseIr(cont);
        mod.writeModuleToFile(kPathToFsrFuseIr + name + kModId);
        size_t numNodesAfter = mod.getReadOnlyModule()->getEntryCircuit()->getNumberOfNodes();
        size_t sizeAfter = std::filesystem::file_size(kPathToFsrFuseIr + name + kModId);

        // for outputs (2)
        out << sizeAfter << kSep
            << numNodesAfter << std::endl;
    }
}


void vectorizeFuseIr(fuse::core::CircuitObjectWrapper& mutableCirc) {
    constexpr int minGates = 64;
    constexpr int maxDepth = 1;

    fuse::passes::vectorizeInstructions(mutableCirc, fuse::core::ir::PrimitiveOperation::Xor, minGates, maxDepth, false);
    fuse::passes::vectorizeInstructions(mutableCirc, fuse::core::ir::PrimitiveOperation::And, minGates, maxDepth, false);
    fuse::passes::vectorizeInstructions(mutableCirc, fuse::core::ir::PrimitiveOperation::Not, minGates, maxDepth, false);
    fuse::passes::vectorizeInstructions(mutableCirc, fuse::core::ir::PrimitiveOperation::Or, minGates, maxDepth, false);
}

void vectorization() {
    std::fstream out(kOutputPath + "vec_sizes_log_64.txt", std::ios::in | std::ios::out | std::ios::app);
    out.seekg(0, std::ios::end);
    if (out.tellg() == 0) {
        // file is emtpy -> add first line
        out << "circuit, size_before, number_of_nodes_before_vec, size_after, number_of_nodes_after_vec" << std::endl;
    }

    for (const auto& name : kToOptimize) {
        fuse::core::CircuitContext cont;
        cont.readCircuitFromFile(kPathToFuseIr + name + kCircId);

        // for outputs
        size_t numNodesBefore = cont.getReadOnlyCircuit()->getNumberOfNodes();
        size_t sizeBefore = std::filesystem::file_size(kPathToFuseIr + name + kCircId);
        out << name << kSep
            << sizeBefore << kSep
            << numNodesBefore << kSep;
        out.flush();

        auto mutableCirc = cont.getMutableCircuitWrapper();
        vectorizeFuseIr(mutableCirc);
        cont.writeCircuitToFile(kPathToVectorizedFuseIr + name + kCircId);

        size_t numNodesAfter = cont.getReadOnlyCircuit()->getNumberOfNodes();
        size_t sizeAfter = std::filesystem::file_size(kPathToVectorizedFuseIr + name + kCircId);

        // for outputs (2)
        out << sizeAfter << kSep
            << numNodesAfter << std::endl;
    }
}

void optimizeFuseIrCircs() {
    for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(kPathToFuseIr)) {
        if (dirEntry.exists() && !dirEntry.path().empty()) {
            std::string circName = dirEntry.path().stem().string();
            if (dirEntry.path().extension() == kCircId) {
                // load circuit for running fsr and producing a module
                {
                    fuse::core::CircuitContext cont;
                    cont.readCircuitFromFile(dirEntry.path().string());
                    auto mod = fsrOnFuseIr(cont);
                    mod.writeModuleToFile(kPathToFsrFuseIr + circName + kModId);
                }
                // run vectorize to produce a vectorized circuit
                {
                    fuse::core::CircuitContext cont;
                    cont.readCircuitFromFile(dirEntry.path().string());
                    auto mutableCirc = cont.getMutableCircuitWrapper();
                    vectorizeFuseIr(mutableCirc);
                    cont.writeCircuitToFile(kPathToVectorizedFuseIr + circName + kCircId);
                }
            } else if (dirEntry.path().extension() == kModId) {
                // load module
                fuse::core::ModuleContext cont;
                cont.readModuleFromFile(dirEntry.path().string());
                auto mutableModule = cont.getMutableModuleWrapper();
                // run vectorize on every circuit to produce a module with vectorized circuits
                for (auto& circ : mutableModule.getAllCircuitNames()) {
                    auto mutableCirc = mutableModule.getCircuitWithName(circ);
                    vectorizeFuseIr(mutableCirc);
                }
                cont.writeModuleToFile(kPathToVectorizedFuseIr + circName + kModId);
            } else {
                std::string error = "unexpected file identifier when optimizing fuse ir circuits for candidate generation: " + dirEntry.path().extension().string();
                throw std::runtime_error(error);
            }
        }
    }
}

void zipBristolCircs() { zipCircs(kPathToBristolCircuits, kPathToZippedBristolCircuits, false); }

void zipFuseIrCircs() { zipCircs(kPathToFuseIr, kPathToZippedFuseIr); }

void zipHyccCircs() {
    for (const auto& hyccCirc : kHyccCircuits) {
        std::filesystem::path p(std::get<0>(hyccCirc));
        const auto name = p.parent_path().filename().string();
        std::filesystem::path hyccCircuitDirectory(p.parent_path());

        std::ifstream cmbContent(p);
        std::stringstream content;

        std::string line;
        while (std::getline(cmbContent, line)) {
            std::ifstream fin(p.parent_path() / line, std::ios::binary);
            content << fin.rdbuf();
        }

        std::string data = content.str();
        std::string compressedData = gzip::compress(data.data(), data.size());
        fuse::core::util::io::writeCompressedStringToBinaryFile(kPathToZippedHyccCircuits + name + kZipId, compressedData);
    }
}

void zipOptimizedFuseIrCircs() {
    zipCircs(kPathToFsrFuseIr, kPathToZippedFsrFuseIr);
    zipCircs(kPathToVectorizedFuseIr, kPathToZippedVectorizedFuseIr);
}
}  // namespace fuse::benchmarks
