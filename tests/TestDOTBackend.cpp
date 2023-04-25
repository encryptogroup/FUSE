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

#include <filesystem>
#include <fstream>

#include "BristolFrontend.h"
#include "DOTBackend.h"
#include "HyCCFrontend.h"
#include "IOHandlers.h"
#include "IR.h"
#include "ModuleGenerator.h"

namespace fuse::tests::backend {

TEST(DISABLED_DOTBackend, HyCCCircuits) {
    std::string output = "../../tests/outputs/dot_output/hycc_circuits_dot.txt";
    std::ofstream of(output);
    std::vector<std::string> hycc = {"../../examples/hycc_circuits/biomatch1k"};
    std::vector<std::string> fb = {"../../tests/outputs/dot_output/biomatch1k.fs"};

    for (size_t test = 0; test < hycc.size(); ++test) {
        if (!std::filesystem::exists(fb.at(test))) {
            ASSERT_NO_THROW(frontend::loadFUSEFromHyCC(hycc.at(test), fb.at(test)));
        }
        core::ModuleContext context;
        context.readModuleFromFile(fb.at(test));

        auto entryCircuit = context.getModuleBufferWrapper().getEntryCircuit();

        of << fuse::backend::generateDotCodeFrom(*entryCircuit);
        of.flush();
    }
}

TEST(DOTBackend, SimpleCircuits) {
    std::string output = "../../tests/outputs/dot_output/simple_circuit_dot.txt";
    std::ofstream of(output);

    std::vector<std::string>
        bristol = {
            "../../examples/bristol_circuits/notGate.bristol",
            "../../examples/bristol_circuits/andGate.bristol",
            "../../examples/bristol_circuits/xorGate.bristol",
            "../../examples/bristol_circuits/twoAndDeep.bristol",
            "../../examples/bristol_circuits/twoAndFlat.bristol",
            "../../examples/bristol_circuits/fullAdder.bristol",
        };

    std::vector<std::string> fb = {
        "../../examples/bristol_circuits/notGate.fs",
        "../../examples/bristol_circuits/andGate.fs",
        "../../examples/bristol_circuits/xorGate.fs",
        "../../examples/bristol_circuits/twoAndDeep.fs",
        "../../examples/bristol_circuits/twoAndFlat.fs",
        "../../examples/bristol_circuits/fullAdder.fs",
    };

    for (size_t test = 0; test < bristol.size(); ++test) {
        ASSERT_NO_THROW(frontend::loadFUSEFromBristolToFile(bristol.at(test), fb.at(test)));
        core::CircuitContext context;
        core::CircuitBufferWrapper circ = context.readCircuitFromFile(fb.at(test));

        of << fuse::backend::generateDotCodeFrom(circ);
        of.flush();
    }
}

TEST(DOTBackend, Calls) {
    std::string output = "../../tests/outputs/dot_output/call_example.txt";
    std::ofstream of(output);
    auto moduleContext = util::generateModuleWithCall();
    auto moduleBuffer = moduleContext.getModuleBufferWrapper();
    of << fuse::backend::generateDotCodeFrom(moduleBuffer);
    of.flush();
}

TEST(DOTBackend, Offsets) {
    std::string output = "../../tests/outputs/dot_output/offset_example.txt";
    std::ofstream of(output);

    frontend::CircuitBuilder cb("test");
    auto st = cb.addDataType(core::ir::PrimitiveType::Bool);
    auto pt = cb.addDataType(core::ir::PrimitiveType::Bool, core::ir::SecurityLevel::Plaintext);
    auto i1 = cb.addInputNode(st);
    auto i2 = cb.addInputNode(st);
    auto i3 = cb.addInputNode(st);
    auto i4 = cb.addInputNode(st);
    auto a = cb.addNode(core::ir::PrimitiveOperation::Merge, {i1, i2, i3, i4, i1, i2, i3, i4});
    auto split = cb.addSplitNode(core::ir::PrimitiveType::UInt8, a);
    std::vector<uint64_t> ins = {split};
    cb.addOutputNode(pt, ins, std::vector<uint32_t>({0}));
    cb.addOutputNode(pt, ins, std::vector<uint32_t>{1});
    cb.addOutputNode(pt, ins, std::vector<uint32_t>{2});
    auto t = cb.addOutputNode(pt, ins, std::vector<uint32_t>{3});
    // cb.addOutputNode(pt, ins, std::vector<uint32_t>{4});
    // cb.addOutputNode(pt, ins, std::vector<uint32_t>{5});
    // cb.addOutputNode(pt, ins, std::vector<uint32_t>{6});
    // cb.addOutputNode(pt, ins, std::vector<uint32_t>{7});
    cb.finish();

    core::CircuitContext ctx(cb);
    auto buf = ctx.getCircuitBufferWrapper();
    of << fuse::backend::generateDotCodeFrom(buf);
    auto testOut = buf.getNodeWithID(t);
    of << testOut->getInputNodeIDs()[0];
    of << testOut->getInputOffsets()[0];
    of.flush();
}

}  // namespace fuse::tests::backend
