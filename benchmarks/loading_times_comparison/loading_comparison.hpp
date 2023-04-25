#ifndef FUSE_COMPARELOADINGTIMES_H
#define FUSE_COMPARELOADINGTIMES_H

#include <BristolFrontend.h>
#include <HyCCFrontend.h>
#include <common/common.h>
#include <libcircuit/simple_circuit.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>

namespace fuse::benchmarks {

/* For all Bristol circuits:
- measure time taken to load FUSE From Bristol
- measure time taken to load that same circuit in already serialized form
*/
void compareLoadingTimesWithBristol() {
    std::ofstream out(kOutputPath + "bristol_loading_comparison.csv");
    constexpr int numExecs = 16;
    constexpr int numLog = 4;  // log2(16)

    out << "circ_name,bristol_frontend_time,load_fuse_time," << std::endl;
    for (const auto& entry : std::filesystem::directory_iterator(kPathToBristolCircuits)) {
        if (entry.exists() && !entry.path().empty() && entry.path().extension() == ".bristol") {
            auto bristolPath = entry.path().string();
            auto circName = entry.path().stem().string();
            auto fusePath = kPathToFuseIr + circName + kCircId;

            // load from bristol
            int64_t bristolTime = 0;
            for (int i = 0; i < numExecs; ++i) {
                auto start = std::chrono::system_clock::now();
                auto circContext = fuse::frontend::loadFUSEFromBristol(bristolPath);
                auto end = std::chrono::system_clock::now();

                auto time = end - start;
                bristolTime += time.count();
            }
            bristolTime >> numLog;  // instead of division, use right shift for normalization

            // load fuse from binary
            int64_t fuseTime = 0;
            for (int i = 0; i < numExecs; ++i) {
                auto start = std::chrono::system_clock::now();
                fuse::core::CircuitContext circContext;
                circContext.readCircuitFromFile(fusePath);
                auto end = std::chrono::system_clock::now();

                auto time = end - start;
                fuseTime += time.count();
            }
            fuseTime >> numLog;  // right shift for normalization

            // write times to csv file
            out << circName << kSep
                << bristolTime << kSep
                << fuseTime << kSep
                << std::endl;
        }
    }
}

void compareLoadingTimesWithHycc() {
    std::ofstream out(kOutputPath + "hycc_loading_comparison.csv");
    constexpr int numExecs = 1;
    constexpr int numLog = 0;  // log2

    out << "circ_name,hycc_parsing_time,hycc_frontend_time,load_fuse_time" << std::endl;
    for (auto hyccCirc : kHyccCircuits) {
        std::filesystem::path p(std::get<0>(hyccCirc));
        const auto circName = p.parent_path().filename().string();
        auto fusePath = kPathToFuseIr + circName + kModId;

        // parse hycc using lambda
        int64_t hycc_parsing_time = 0;
        for (int i = 0; i < numExecs; ++i) {
            auto start = std::chrono::system_clock::now();
            auto res_map = loadHyccFromCircFile(std::get<0>(hyccCirc));
            auto end = std::chrono::system_clock::now();
            std::cout << res_map.size() << std::endl;

            auto time = end - start;
            hycc_parsing_time += time.count();
        }
        hycc_parsing_time >> numLog;  // instead of division, use right shift for normalization

        // parse + translate to fuse ir
        int64_t hycc_frontend_time = 0;
        for (int i = 0; i < numExecs; ++i) {
            auto start = std::chrono::system_clock::now();
            auto modContext = fuse::frontend::loadFUSEFromHyCCWithCalls(std::get<0>(hyccCirc), std::get<1>(hyccCirc));
            auto end = std::chrono::system_clock::now();

            auto time = end - start;
            hycc_frontend_time += time.count();
        }
        hycc_frontend_time >> numLog;  // instead of division, use right shift for normalization

        // load serialized fuse ir directly from binary
        int64_t load_fuse_time = 0;
        for (int i = 0; i < numExecs; ++i) {
            auto start = std::chrono::system_clock::now();
            fuse::core::ModuleContext modContext;
            modContext.readModuleFromFile(fusePath);
            auto end = std::chrono::system_clock::now();
            auto time = end - start;
            load_fuse_time += time.count();
        }
        load_fuse_time >> numLog;  // right shift for normalization

        // write times to csv file
        out << circName << kSep
            << hycc_parsing_time << kSep
            << hycc_frontend_time << kSep
            << load_fuse_time << kSep
            << std::endl;
    }
}

}  // namespace fuse::benchmarks
#endif /* FUSE_COMPARELOADINGTIMES_H */
