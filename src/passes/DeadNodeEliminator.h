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

#ifndef FUSE_DEADNODEELIMINATOR_H
#define FUSE_DEADNODEELIMINATOR_H

#include "ModuleWrapper.h"

namespace fuse::passes {

/**
 * \brief Executes a dead node elimination pass on every circuit inside the module.
 *
 * A node of any circuit inside the module is considered dead if it is not used to compute
 * any of the outputs from the module's entry_point circuit.
 *
 * @param module mutable module where all the circuits' dead nodes are removed
 * @param removeUnusedCircuits removes circuits which are not called to compute any of the entry point's outputs.
 */
void eliminateDeadNodes(core::ModuleObjectWrapper& module, bool removeUnusedCircuits = false);

void eliminateDeadNodes(core::CircuitObjectWrapper& circuit);

}  // namespace fuse::passes

#endif /* FUSE_DEADNODEELIMINATOR_H */
