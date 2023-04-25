#ifndef FUSE_COMPAREFILESIZES_H
#define FUSE_COMPAREFILESIZES_H

#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>

#include "common/common.h"

namespace fuse::benchmarks {

void compareBristolCircuitSizes() {
    std::ofstream out(kOutputPath + "bristol_size_comparison_fsr.csv");
    // out << "circ_name, bristol_size, fuse_size, optimized_fuse_size, zipped_bristol_size, zipped_fuse_size, zipped_optimized_fuse_size\n";
    out << "circ_name, bristol_size, fuse_size, zipped_bristol_size, zipped_fuse_size" << std::endl;


    for (const auto& candidate : kToOptimize) {
        size_t bristolSize = std::filesystem::file_size(kPathToBristolCircuits + candidate + ".bristol");
        size_t fuseSize = std::filesystem::file_size(kPathToFuseIr + candidate + kCircId);
        // size_t fsrSize = std::filesystem::file_size(kPathToFsrFuseIr + candidate + kCircId);
        // size_t vectSize = std::filesystem::file_size(kPathToVectorizedFuseIr + circ);

        size_t zippedBristolSize = std::filesystem::file_size(kPathToZippedBristolCircuits + candidate + ".z");
        size_t zippedFuseSize = std::filesystem::file_size(kPathToZippedFuseIr + candidate + kCircId + kZipId);
        // size_t zippedFsrFuseSize = std::filesystem::file_size(kPathToZippedFsrFuseIr + candidate + kCircId + kZipId);

        // size_t zippedVectFuseSize = std::filesystem::file_size(kPathToZippedVectorizedFuseIr + circ);

        out << candidate << kSep
            << bristolSize << kSep
            << fuseSize << kSep
            // << fsrSize << kSep
            // << vectSize << kSep
            << zippedBristolSize << kSep
            << zippedFuseSize << kSep
            // << zippedFsrFuseSize << kSep
            // << zippedVectFuseSize << kSep
            << std::endl;
    }
}

void compareOwnSha256Sizes() {
    size_t fuseSize = std::filesystem::file_size(kPathToFuseIr + "OWN_SHA256" + kModId);

    auto data = fuse::core::util::io::readFlatBufferFromBinary(kPathToFuseIr + "OWN_SHA256" + kModId);
    auto compressedData = gzip::compress(data.data(), data.size());
    fuse::core::util::io::writeCompressedStringToBinaryFile(kPathToZippedFuseIr + "OWN_SHA256" + kModId + ".z", compressedData);

    size_t zippedFuseSize = std::filesystem::file_size(kPathToZippedFuseIr + "OWN_SHA256" + kModId + kZipId);

    std::cout << "OWN_SHA256" << kSep
              << fuseSize << kSep
              << zippedFuseSize << kSep
              << std::endl;
}

void compareHyccCircuitSizes() {
    std::ofstream out(kOutputPath + "hycc_size_comparison.csv");
    out << "circ_name, sum_hycc_size, fuse_size, zipped_hycc_size, zipped_fuse_size\n";

    for (const auto& hyccCircTuple : kHyccCircuits) {
        // std::filesystem::path;
        std::filesystem::path p(std::get<0>(hyccCircTuple));
        const auto name = p.parent_path().filename().string();
        std::filesystem::path hyccCircuitDirectory(p.parent_path());

        size_t hyccSize = 0;
        std::ifstream cmbContent(p);
        std::string line;
        while (std::getline(cmbContent, line)) {
            hyccSize += std::filesystem::file_size(hyccCircuitDirectory / line);
        }

        size_t fuseSize = std::filesystem::file_size(kPathToFuseIr + name + kModId);
        size_t zippedFuseSize = std::filesystem::file_size(kPathToZippedFuseIr + name + kModId + kZipId);
        size_t zippedHyccSize = std::filesystem::file_size(kPathToZippedHyccCircuits + name + kZipId);

        out << name << ", "
            << hyccSize << ", "
            << fuseSize << ", "
            << zippedHyccSize << ", "
            << zippedFuseSize << ", "
            << std::endl;
    }
}

}  // namespace fuse::benchmarks
#endif /* FUSE_COMPAREFILESIZES_H */
