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

#ifndef FUSE_IOHANDLERS_H
#define FUSE_IOHANDLERS_H

#include <array>
#include <string>
#include <vector>

namespace fuse::core::util::io {

std::vector<char> readFlatBufferFromBinary(const std::string &pathToRead);

std::string readTextFile(const std::string &pathToRead);

// std::vector<char> readFlatBufferFromJSON(const std::string &pathToRead);

void writeFlatBufferToBinaryFile(const std::string &pathToWrite, const std::vector<char> &buffer);

void writeStringToTextFile(const std::string &pathToWrite, const std::string &content);

void writeFlatBufferToBinaryFile(const std::string &pathToWrite, uint8_t *bufferPointer, long bufferSize);

void writeCompressedStringToBinaryFile(const std::string &pathToWrite, const std::string &content);

// void writeFlatBufferToJSON(const std::string &pathToWrite, char *bufPointer, size_t bufSize);
// void writeFlatBufferToJSON(const std::string &pathToWrite, std::vector<char> buffer);
// size_t getFileSize(const std::string &filePath);

}  // namespace fuse::core::util::io

#endif  // FUSE_IOHANDLERS_H
