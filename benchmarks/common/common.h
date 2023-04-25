#ifndef FUSE_BENCHMARKSCOMMON_H
#define FUSE_BENCHMARKSCOMMON_H

#include <libcircuit/simple_circuit.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "IR.h"

namespace fuse::benchmarks {

std::string getNameFromPath(const std::string& path, bool hasFileSuffix = true);

inline double improvement(double unoptimized, double optimized) {
    return 100 * (1 - (optimized / unoptimized));
}

void generateFuseFromBristol();
void generateFuseFromHycc();
void optimizeFuseIrCircs();
void fsr();
void vectorization();
void zipBristolCircs();
void zipFuseIrCircs();
void zipOptimizedFuseIrCircs();
void zipHyccCircs();


// Related to SHA generation in FUSE
constexpr int wordSize = 32;
constexpr int byteSize = 8;
using pt = fuse::core::ir::PrimitiveType;
using op = fuse::core::ir::PrimitiveOperation;
using Id = fuse::frontend::Identifier;
using Offset = unsigned int;
using type = fuse::core::ir::PrimitiveType;
void generateSHA256();

std::unordered_map<std::string, simple_circuitt> loadHyccFromCircFile(const std::string& pathToHyccCirc);

const std::string kPathToBristolCircuits = "../../../examples/bristol_circuits/";
const std::string kPathToZippedBristolCircuits = "../../../benchmarks/resources/bristol_zipped/";
const std::string kPathToZippedHyccCircuits = "../../../benchmarks/resources/hycc_zipped/";
const std::string kPathToFuseIr = "../../../benchmarks/resources/fuse_ir/";
const std::string kPathToZippedFuseIr = "../../../benchmarks/resources/fuse_ir_zip/";
const std::string kPathToFsrFuseIr = "../../../benchmarks/resources/fuse_ir_fsr/";
const std::string kPathToZippedFsrFuseIr = "../../../benchmarks/resources/fuse_ir_fsr_zip/";
const std::string kPathToZippedVectorizedFuseIr = "../../../benchmarks/resources/fuse_ir_vect_zip/";
// Vectorization Path
const std::string kPathToVectorizedFuseIr = "../../../benchmarks/resources/fuse_ir_vect_64/";
// for MOTION: GMW paths
const std::string kPathToGreedyVect = "../../../benchmarks/resources/fuse_ir_vect_greedy/";
const std::string kPathToVect8 = "../../../benchmarks/resources/fuse_ir_vect_8/";
const std::string kPathToVect16 = "../../../benchmarks/resources/fuse_ir_vect_16/";
const std::string kPathToVect32 = "../../../benchmarks/resources/fuse_ir_vect_32/";
const std::string kPathToVect64 = "../../../benchmarks/resources/fuse_ir_vect_64/";


const std::string kCircId = ".cfs";
const std::string kModId = ".mfs";
const std::string kZipId = ".z";

const std::vector<std::tuple<std::string, std::string>> kHyccCircuits = {
    {"../../../examples/hycc_circuits/tutorial_addition/all.cmb", "mpc_main"},
    {"../../../examples/hycc_circuits/tutorial_euclidean_distance/all.cmb", "mpc_main"},
    {"../../../examples/hycc_circuits/gauss/all.cmb", "mpc_main"},
    {"../../../examples/hycc_circuits/biomatch1k/yaohybrid.cmb", "mpc_main"},
    {"../../../examples/hycc_circuits/biomatch4k/yaohybrid.cmb", "mpc_main"},
    {"../../../examples/hycc_circuits/cryptonets/yaohybrid.cmb", "mpc_main"},
    {"../../../examples/hycc_circuits/kmeans/yaohybrid.cmb", "mpc_main"},
    {"../../../examples/hycc_circuits/mnist/yaohybrid.cmb", "mpc_main"}};


const std::vector<std::string>
    kToOptimize = {
        // < 200 nodes, timeout (s) : 20, try_modes: 2
        "int_add8_size",
        "int_sub8_size",
        "int_add8_depth",
        "int_gt8_size",
        "int_gt8_depth",
        "int_sub8_depth",
        // > 120
        "int_add16_size",
        "int_sub16_size",
        "int_gt16_size",
        "int_gt16_depth",
        "int_add16_depth",
        "int_mul8_size",
        "int_sub16_depth",
        "int_mul8_depth",
        "int_add32_size",
        "int_sub32_size",
        "int_gt32_size",
        // 200 < #nodes < 450
        "int_gt32_depth",
        "int_add32_depth",
        "int_div8_size",
        "int_add64_size",
        "int_sub32_depth",
        "int_div8_depth",
        "int_sub64_size",
        "int_gt64_size",
        "int_gt64_depth",
        // 640 < #nodes < 780
        "int_mul16_size",
        "int_mul16_depth",
        "int_add64_depth",
        "int_sub64_depth",
        // 1000 < #nodes < 1620
        "int_div16_size",
        "int_div16_depth",
        "FP-eq",
        "FP-lt",
        // 2800 < #nodes < 4000
        "FP-ceil",
        "int_mul32_size",
        "int_mul32_depth",
        "int_div32_size",
        "int_div32_depth",
        "FP-f2i",
        // (7000, 15650)
        "FP-i2f",
        "int_mul64_size",
        "int_mul64_depth",
        "int_div64_size",
        "int_div64_depth",
        "FP-add",
        // (36600, 77900)
        "aes_128",
        "aes_192",
        "FP-mul",
        "aes_256",
        "md5",
        // (135000, 350000)
        "sha_256",
        "FP-div",
        "Keccak_f",
        "FP-sqrt",
        "sha_512",
};

const std::string kOutputPath = "../../../benchmarks/outputs/";
const std::string kSep = ", ";

}  // namespace fuse::benchmarks
#endif /* FUSE_BENCHMARKSCOMMON_H */
