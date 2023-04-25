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

#include <BristolFrontend.h>
#include <IOHandlers.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <gzip/compress.hpp>
#include <regex>

#include "BristolFormatBackend.h"
#include "ModuleGenerator.h"

namespace fuse::tests::frontend {

TEST(DISABLED_BristolFrontendTest, CompareFileSizes) {
    namespace io = fuse::core::util::io;

    std::string pathToBristolCircuits = "../../examples/bristol_circuits";
    std::string outputPath = "../../tests/outputs/bristol_circuit_comparison/";

    std::ofstream csvOutput;

    time_t t = time(nullptr);  // get time now
    struct tm *now = localtime(&t);
    char buffer[80];
    strftime(buffer, 80, "%d-%m-%y:%T", now);

    csvOutput.open(outputPath + "motion_circuit_sizes_" + buffer + ".csv");
    csvOutput << "Name, Bristol Size, Zipped Bristol Size, Binary Size, Zipped Binary Size, Binary/Bristol, Zipped Binary/Zipped Bristol, Binary/Zipped Binary\n";

    for (const auto &dirEntry : std::filesystem::recursive_directory_iterator(pathToBristolCircuits)) {
        // filter for the .bristol files
        if (dirEntry.exists() && !dirEntry.path().empty() && dirEntry.path().extension() == ".bristol") {
            // split path to bristol file into name and path
            auto fileName = dirEntry.path().filename();
            auto bristolPath = dirEntry.path().string();

            // set output paths for bristol input and generated files
            auto goalPath = outputPath + fileName.string();
            auto binaryPath = std::regex_replace(goalPath, std::regex("\\.bristol"), ".fs");
            auto compressedBinaryPath = binaryPath + ".z";
            auto compressedBristolPath = bristolPath + ".z";

            // compute the size of the original bristol file
            auto bristolSize = std::filesystem::file_size(dirEntry);

            // zip the contents of the bristol file to calculate the compressed bristol size
            std::string bristolContents = io::readTextFile(bristolPath);
            std::string compressedBristol = gzip::compress(bristolContents.data(), bristolContents.size());
            ASSERT_NO_THROW(io::writeCompressedStringToBinaryFile(compressedBristolPath, compressedBristol));
            auto zippedBristolSize = std::filesystem::file_size(compressedBristolPath);

            // translate bristol -> FUSE binary and calculate the binary size
            ASSERT_NO_THROW(fuse::frontend::loadFUSEFromBristolToFile(bristolPath, binaryPath));
            auto binarySize = std::filesystem::file_size(binaryPath);

            // zip the contents of the binary to calculate the compressed binary size
            std::vector<char> bufferContent = io::readFlatBufferFromBinary(binaryPath);
            std::string compressedBinary = gzip::compress(bufferContent.data(), bufferContent.size());
            io::writeCompressedStringToBinaryFile(compressedBinaryPath, compressedBinary);
            auto compressedSize = std::filesystem::file_size(compressedBinaryPath);

            // write all the calculated sizes to the csv file
            csvOutput << fileName << ", ";
            csvOutput << bristolSize << ",";
            csvOutput << zippedBristolSize << ",";
            csvOutput << binarySize << ",";
            csvOutput << compressedSize << ",";
            csvOutput << (double)binarySize / (double)bristolSize << ",";
            csvOutput << (double)compressedSize / (double)zippedBristolSize << ",";
            csvOutput << (double)binarySize / (double)compressedSize << "\n";
        }
    }
}

TEST(DISABLED_BristolFrontendTest, SHA512) {
    namespace io = fuse::core::util::io;
    std::string outputPath = "../../tests/outputs/bristol_circuit_comparison/";
    std::ofstream csvOutput;
    time_t t = time(nullptr);  // get time now
    struct tm *now = localtime(&t);
    char buffer[80];
    strftime(buffer, 80, "%d-%m-%y:%T", now);
    csvOutput.open(outputPath + "sha512_sizes" + buffer + ".csv");
    csvOutput << "Name, Bristol Size, Zipped Bristol Size, FUSE Size, Zipped FUSE Size, FUSE/Bristol, Zipped FUSE/Zipped FUSE, FUSE/Zipped FUSE\n";

    for (unsigned i = 1; i <= 100; ++i) {
        auto modContext = fuse::util::generateModuleWithSha512Calls(i);
        std::size_t bristolSize;
        std::size_t zippedBristolSize;
        std::size_t fuseSize;
        std::size_t zippedFuseSize;

        {
            auto bristolString = fuse::backend::generateBristolFormatFrom(modContext.getModuleBufferWrapper());
            auto compressedBristol = gzip::compress(bristolString.data(), bristolString.size());
            bristolSize = bristolString.size();
            zippedBristolSize = compressedBristol.size();
        }
        {
            fuseSize = sizeof(modContext);
            auto compressedFuse = gzip::compress(modContext.getBufferPointer(), modContext.getBufferSize());
            zippedFuseSize = compressedFuse.size();
        }
        // write all the calculated sizes to the csv file
        csvOutput << i << "_calls, ";
        csvOutput << bristolSize << ",";
        csvOutput << zippedBristolSize << ",";
        csvOutput << fuseSize << ",";
        csvOutput << zippedFuseSize << ",";
        csvOutput << (double)fuseSize / (double)bristolSize << ",";
        csvOutput << (double)zippedFuseSize / (double)zippedBristolSize << ",";
        csvOutput << (double)fuseSize / (double)zippedFuseSize << "\n";
        csvOutput.flush();
    }
}

}  // namespace fuse::tests::frontend
