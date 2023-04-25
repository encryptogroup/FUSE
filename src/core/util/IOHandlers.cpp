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

#include "IOHandlers.h"

#include <fstream>

namespace fuse::core::util::io {

/*
 * Read
 */

std::string readTextFile(const std::string &pathToRead) {
    std::ifstream inputFile(pathToRead);
    std::string result((std::istreambuf_iterator<char>(inputFile)),
                       std::istreambuf_iterator<char>());
    return result;
}

std::vector<char> readFlatBufferFromBinary(const std::string &pathToRead) {
    std::ifstream inputFile(pathToRead, std::ios::in | std::ios::binary);
    std::vector<char> data((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
    return data;
}

/*
 * Write
 */

void writeFlatBufferToBinaryFile(const std::string &pathToWrite, const std::vector<char> &buffer) {
    std::ofstream outputFileStream;
    outputFileStream.open(pathToWrite, std::ios::out | std::ios::binary);
    outputFileStream.write(buffer.data(), buffer.size());
    outputFileStream.close();
}

void writeStringToTextFile(const std::string &pathToWrite, const std::string &content) {
    std::ofstream outputFileStream;
    outputFileStream.open(pathToWrite, std::ios::out);
    outputFileStream.write(content.data(), content.size());
    outputFileStream.close();
}

void writeFlatBufferToBinaryFile(const std::string &pathToWrite, uint8_t *bufferPointer, long bufferSize) {
    std::ofstream outputFileStream;
    outputFileStream.open(pathToWrite, std::ios::out | std::ios::binary);
    outputFileStream.write(reinterpret_cast<const char *>(bufferPointer), bufferSize);
    outputFileStream.close();
}

void writeCompressedStringToBinaryFile(const std::string &pathToWrite, const std::string &content) {
    std::ofstream outputFileStream;
    outputFileStream.open(pathToWrite, std::ios::out | std::ios::binary);
    outputFileStream << content;
    outputFileStream.close();
}

}  // namespace fuse::core::util::io
