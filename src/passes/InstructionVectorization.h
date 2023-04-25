/*
 * MIT License
 *
 * Copyright (c) 2022 Moritz Huppert
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

#ifndef FUSE_INSTRUCTIONVECTORIZATION_H
#define FUSE_INSTRUCTIONVECTORIZATION_H

#include "ModuleWrapper.h"

namespace fuse::passes {

void vectorizeInstructions(core::CircuitObjectWrapper& circuit, core::ir::PrimitiveOperation operationType, int minGates = 2, int maxDistance = 100, bool multi=false);

void vectorizeAllInstructions(core::CircuitObjectWrapper& circuit, int minGates = 2, int maxDistance = 100);

}  // namespace fuse::passes

#endif /* FUSE_INSTRUCTIONVECTORIZATION_H */
