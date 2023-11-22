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

#include <chrono>
#include <filesystem>
#include <iostream>

#include "ConstantFolder.h"
#include "DeadNodeEliminator.h"
#include "IOHandlers.h"
#include "ModuleGenerator.h"
#include "NodeSuccessorsAnalysis.h"

namespace fuse::tests::core {

// num nodes : size in B
// 1 000 000 : 58 162 240
// 2 000 000 : 116 586 240 ~ 100 MB
// 250 MB   ~> 5 000 000
// 500 MB   ~> 10 000 000
// 1 GB     ~> 20 000 000
// 2 GB     ~> 40 000 000
// 3 GB     ~> 60 000 000
// 4 GB     ~> 80 000 000
// 5 GB     ~>
// 6 GB     ~>
// 7 GB     ~>
// 8 GB     ~>
// 9 GB     ~>
// 10 GB     ~>

TEST(LargeCircuits, BuildAnalyzeTransform) {
    std::cout << "Number of Nodes, Binary Size, Time taken for serialization (ns), Time in (s)\n";
    std::vector<long> numberOfNodes{
        5000000,
        10000000,
        20000000,
        40000000,
        60000000,
        80000000,
        100000000,
        120000000,
        140000000,
        160000000,
        180000000,
        200000000,
    };
    for (long i : numberOfNodes) {
        std::cout << i << ", ";
        std::cout.flush();
        std::size_t binarySize;
        std::size_t compressedSize;
        int64_t milliSecs;
        {
            auto startTime = std::chrono::steady_clock::now();
            fuse::core::CircuitContext circ = fuse::util::generateCircuitWithNumberOfNodes(i);
            auto endTime = std::chrono::steady_clock::now();
            binarySize = circ.getBinarySize();
            milliSecs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
            std::cout << binarySize << ", ";
            std::cout << milliSecs << " ms" << std::endl;

            {
                startTime = std::chrono::steady_clock::now();
                fuse::core::CircuitBufferWrapper bufWrapper = circ.getCircuitBufferWrapper();
                endTime = std::chrono::steady_clock::now();
                milliSecs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
                std::cout << "[INFO] Buffer Wrapper Access in: " << milliSecs << " ms" << std::endl;

                startTime = std::chrono::steady_clock::now();
                fuse::passes::getNodeSuccessors(bufWrapper);
                endTime = std::chrono::steady_clock::now();
                milliSecs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
                std::cout << "[SUCCESS] Node Successors Analysis: " << milliSecs << " ms" << std::endl;
            }

            {
                startTime = std::chrono::steady_clock::now();
                fuse::core::CircuitObjectWrapper objWrapper = circ.getMutableCircuitWrapper();
                endTime = std::chrono::steady_clock::now();
                milliSecs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
                std::cout << "[INFO] Unpacking + Object Wrapper Access in: " << milliSecs << " ms" << std::endl;

                startTime = std::chrono::steady_clock::now();
                fuse::passes::foldConstantNodes(objWrapper);
                endTime = std::chrono::steady_clock::now();
                milliSecs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
                std::cout << "[SUCCESS] Constant Folding: " << milliSecs << " ms" << std::endl;

                startTime = std::chrono::steady_clock::now();
                fuse::passes::eliminateDeadNodes(objWrapper);
                endTime = std::chrono::steady_clock::now();
                milliSecs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
                std::cout << "[SUCCESS] Dead Node Elimination: " << milliSecs << " ms" << std::endl;
            }
        }
    }
}

}  // namespace fuse::tests::core
