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

#ifndef FUSE_BRISTOLFRONTEND_H
#define FUSE_BRISTOLFRONTEND_H

#include <stdexcept>
#include <string>

#include "IR.h"
#include "ModuleWrapper.h"

namespace fuse::frontend {

struct bristol_error : std::runtime_error {
    using std::runtime_error::runtime_error;
};

void loadFUSEFromBristolToFile(const std::string &bristolInputFilePath, const std::string &outputBufferPath = "");

core::CircuitContext loadFUSEFromBristol(const std::string &bristolInputFilePath);

}  // namespace fuse::frontend

#endif  // FUSE_BRISTOLFRONTEND_H
