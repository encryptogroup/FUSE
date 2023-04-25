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

/*
This file shows some example usages of FUSE for developers who want to extend
this framework with more passes, frontend languages or backend languages.
 */
#include <iostream>

#include "BristolFrontend.h"
#include "IR.h"
#include "ModuleWrapper.h"

int main() {
    // load FUSE from Bristol Format: this creates a flatbuffer and embeds it in a circuit context
    // The context manages access to the FUSE circuit by providing a suitable wrapper around the flatbuffers data
    fuse::core::CircuitContext context = fuse::frontend::loadFUSEFromBristol("../../examples/bristol_circuits/md5.bristol");

    /*
    Because the underlying data is organized by flatbuffers which can be tricky to use, there are wrappers available
    that emulate the same behavior by calling the appropriate flatbuffers function. There are multiple ways to access the data:
    (1) You can get a so-called BufferWrapper that directly reads from the flatbuffer, so the circuit does not need to be unpacked
    (2) If you want to perform mutations of any sort, you can get a so-called MutableWrapper.
        This unpacks the complete circuit first and then you can perform arbitrary transformations by calling the according wrapper functions.
        Note that when having unpacked a circuit, the flatbuffers and unpacked data are not in-sync, meaning that whenever you mutate anything,
        this is not reflected by the flatbuffers data but only by unpacked data.
    (3) If you only want to read the data but do not know whether the circuit has been unpacked or not, you can get a pointer to either of the both wrappers
        by calling getReadOnlyCircuit(). This function checks if the data has been unpacked and returns an according wrapper. This way, you will always
        get a wrapper that supports read access without accidentally unpacking the data.
     */

    {
        // Using the Buffer Wrapper, you can access the underlying data without unpacking the whole circuit
        auto readonlyBufferWrapper = context.getCircuitBufferWrapper();
        std::cout << "Hello World, my name is: " << readonlyBufferWrapper.getName() << std::endl;
        std::cout << "I have a total of " << readonlyBufferWrapper.getNumberOfNodes() << " nodes, cool right?" << std::endl;
        // End of lifetime for readonlyBufferWrapper to avoid having both wrapper types starting from line 32
    }

    // if you want to make changes to the circuit, you have to get a mutable circuit wrqapper
    auto writeableCircuit = context.getMutableCircuitWrapper();
    writeableCircuit.setName("CoolMD5");
    // Note that the readonly circuit here points to the mutable circuit
    // At this point, trying to get a direct BufferWrapper will result in an error, as the data has been unpacked already
    std::cout << "I changed my name to " << context.getReadOnlyCircuit()->getName() << std::endl;

    // Both Wrappers both declare begin and end functions for their nodes
    // So if you want to traverse the Nodes for simple operations like counting the #AND gates, just use a for each loop
    size_t numberOfAndNodes = 0;
    for (auto node : writeableCircuit) {
        if (node.getOperation() == fuse::core::ir::PrimitiveOperation::And) {
            ++numberOfAndNodes;
        }
    }
    std::cout << "I have a total of " << numberOfAndNodes << " AND Nodes in the circuitry, wow!" << std::endl;

    // If you don't know which Wrapper you can use for read access, use getReadOnlyCircuit()
    // However, the abstract return type does not declare begin() and end() anymore, so traversing can be achieved like this:
    size_t anotherNumberOfAndNodes = 0;
    auto readOnlyCircuit = context.getReadOnlyCircuit();
    readOnlyCircuit->topologicalTraversal(
        [=, &anotherNumberOfAndNodes](fuse::core::NodeReadOnly& node) {
            if (node.getOperation() == fuse::core::ir::PrimitiveOperation::And) {
                ++anotherNumberOfAndNodes;
            }
        });
    std::cout << "With another method of traversing, I still have " << anotherNumberOfAndNodes << " AND Nodes in the circuitry, wow!" << std::endl;

    // we can save the changes by writing the circuit to file
    context.writeCircuitToFile("../../examples/bristol_circuits/md5.fs");
}
