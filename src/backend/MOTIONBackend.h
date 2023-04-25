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
#ifndef FUSE_MOTIONBACKEND_H
#define FUSE_MOTIONBACKEND_H

#include <variant>

// FUSE
#include "BaseVisitor.h"
#include "ModuleWrapper.h"

// MOTION
#include "base/party.h"
#include "protocols/share_wrapper.h"

namespace fuse::backend {

using Identifier = uint64_t;
using Share = encrypto::motion::ShareWrapper;
using ShareVector = std::vector<encrypto::motion::ShareWrapper>;

/*
Goal: Get an input mapping, then pass that mapping over to MOTION by generating the corresponding Input Values
(inpuMap) -> MOTION Output Shares which can then be evaluated or so?
 */
class MOTIONBackend : fuse::core::BaseReadOnlyVisitor {
   public:
    struct motion_error : std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    explicit MOTIONBackend(std::unordered_map<Identifier, std::variant<encrypto::motion::ShareWrapper, ShareVector>>& inputShares) : nodesToMotionShare(inputShares) {}
    MOTIONBackend() = default;

    virtual void visit(const core::NodeReadOnly& node) override;

    ShareVector evaluate(const core::CircuitReadOnly& circuit);

    virtual void visit(const core::ModuleReadOnly& module) override;

    void setInputShares(std::unordered_map<Identifier, std::variant<encrypto::motion::ShareWrapper, ShareVector>> inputShares) { nodesToMotionShare = inputShares; }

   private:
    ShareVector getInputSharesForNode(const core::NodeReadOnly& node) const;

    std::unordered_map<Identifier, std::variant<encrypto::motion::ShareWrapper, ShareVector>> nodesToMotionShare;
};

std::unordered_map<Identifier, ShareVector> evaluate(const core::CircuitReadOnly& circuit, const encrypto::motion::PartyPointer& party);

/**

 * @brief Evaluates the main circuit with MOTION using the module to resolve calls.
 *
 * @param module the module that contains the called subcircuits
 * @param party MOTION party
 * @param main the circuit to evaluate
 * @return std::unordered_map<Identifier, ShareVector> a mapping for each output node in main to the evaluated vector of shares
 */
std::unordered_map<Identifier, ShareVector> evaluate(const core::ModuleReadOnly& module,
                                                     const encrypto::motion::PartyPointer& party,
                                                     const core::CircuitReadOnly& main);

}  // namespace fuse::backend

#endif /* FUSE_MOTIONBACKEND_H */
