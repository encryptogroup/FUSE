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

#ifndef FUSE_HYCCFRONTEND_H
#define FUSE_HYCCFRONTEND_H

#include <stdexcept>
#include <string>

#include "IR.h"

namespace fuse::frontend {

class hycc_error : std::runtime_error {
    using std::runtime_error::runtime_error;
};

[[deprecated]] void loadFUSEFromHyCC(const std::string &circuitDirectory, const std::string &outputBufferPath = "");

void loadFUSEFromHyCCAndSaveToFile(const std::string &pathToCmbFile, const std::string &outputBufferPath, const std::string &entryCircuitName = "mpc_main");

core::ModuleContext loadFUSEFromHyCCWithCalls(const std::string &pathToCmbFile, const std::string &entryCircuitName = "mpc_main");

}  // namespace fuse::frontend

#endif  // FUSE_HYCCFRONTEND_H
