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

#ifndef FUSE_FREQUENTSUBCIRCUITREPLACEMENT_H
#define FUSE_FREQUENTSUBCIRCUITREPLACEMENT_H

#include "IR.h"
#include "ModuleWrapper.h"

namespace fuse::passes {

/**
 * 3 different modes are supported
 * 0: replace largest patterns
 * 1: replace second largest patterns
 * 2: replace third largest patterns
 */
fuse::core::ModuleContext replaceFrequentSubcircuits(fuse::core::CircuitContext& circuitContext, int frequency_threshold = 3, int mode = 0, std::string distgraphpath = "", std::string glasgowpath = "");

fuse::core::ModuleContext automaticallyReplaceFrequentSubcircuits(fuse::core::CircuitContext& circuitContext, int try_modes = 1, int timeoutseconds = 1, int pattern_upper = 20, int pattern_lower = 2);

}  // namespace fuse::passes

#endif /* FUSE_FREQUENTSUBCIRCUITREPLACEMENT_H */
