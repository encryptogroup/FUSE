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

#ifndef FUSE_MODULEGENERATOR_H
#define FUSE_MODULEGENERATOR_H

#include "IR.h"

namespace fuse::util {

/**
 * @brief Generates a circuit with the specified number of nodes in total
 *
 * @param totalNumberOfNodes the total number of nodes the circuit shall contain
 * @return core::CircuitContext
 */
core::CircuitContext generateCircuitWithNumberOfNodes(unsigned long totalNumberOfNodes);

/**
 * @brief Generate module that calls the SHA-512 Bristol implementation.
 *
 * @param numberOfShaCalls the total number of calls to the SHA-512 functionality.
 * @return core::ModuleContext
 */
core::ModuleContext generateModuleWithSha512Calls(unsigned numberOfShaCalls);

/**
 *
 * @brief [NOT YET IMPLEMENTED] Generates a circuit that contains foldable constant expressions.
 *
 * @param totalNumberOfNodes the total number of nodes the circuit shall contain
 * @return core::CircuitContextπ
 */
core::CircuitContext generateCircuitWithFoldableExpressions(unsigned long totalNumberOfNodes);

/**
 *  @brief [NOT YET IMPLEMENTED] Generates a circuit that contains dead nodes.
 *
 * @param totalNumberOfNodes the total number of nodes the circuit shall contain
 * @return core::CircuitContext
 */
core::CircuitContext generateCircuitWithDeadNodes(unsigned long totalNumberOfNodes);

core::ModuleContext generateModuleWithCall();

}  // namespace fuse::util

#endif /* FUSE_MODULEGENERATOR_H */
