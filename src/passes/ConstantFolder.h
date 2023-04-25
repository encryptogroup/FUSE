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

#ifndef FUSE_CONSTANTFOLDER_H
#define FUSE_CONSTANTFOLDER_H

#include "ModuleWrapper.h"

namespace fuse::passes {

/**
 * @brief Executes constant folding on every circuit inside the module.
 *
 * It runs a dead code elimination first, then performs constant folding,
 * and runs dead code elimination again to clean the circuit after having replaced nodes with other nodes.
 *
 * @param module mutable module where the constant folding is performed
 */
void foldConstantNodes(core::ModuleObjectWrapper& module);

void foldConstantNodes(core::CircuitObjectWrapper& circuit);

}  // namespace fuse::passes

#endif /* FUSE_CONSTANTFOLDER_H */
