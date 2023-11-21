#include "OperationsAnalysis.h"

namespace fuse::passes {

std::unordered_map<std::string, int> analyzeOperations(const core::CircuitReadOnly& circ) {
    std::unordered_map<std::string, int> calc_res;

    circ.topologicalTraversal([&](const core::NodeReadOnly& node){
        calc_res[node.getOperationName()]++;
    });
    return calc_res;
}

std::unordered_map<std::string, std::unordered_map<std::string, int>> analyzeOperations(const core::ModuleReadOnly& mod) {
    std::unordered_map<std::string, std::unordered_map<std::string, int>> result;

    const auto& circs = mod.getAllCircuitNames();
    for (const auto& circName : circs) {
        const auto& key = mod.getCircuitWithName(circName);
        const auto val = analyzeOperations(*key);
        result[key->getName()] = val;
    }

    return result;
}

} // namespace fuse::passes
