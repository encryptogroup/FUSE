#
# MIT License
#
# Copyright (c) 2022 Nora Khayata
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

from operator import neg
import sys
sys.path.append("../../../extern/MP-SPDZ")
sys.path.append("../../core/fuse_ir_fb_headers")

# FUSE IR
import fuse.core.ir.ModuleTable as mod
import fuse.core.ir.CircuitTable as circ
from fuse.core.ir.NodeTable import NodeTable
from fuse.core.ir.DataTypeTable import DataTypeTable
from fuse.core.ir.PrimitiveOperation import PrimitiveOperation as primop
from fuse.core.ir.PrimitiveType import PrimitiveType as primtype

from functools import reduce

# FlatBuffers
import flatbuffers
import flatbuffers.flexbuffers as flexbuffers

# MP-SPDZ
import Compiler
# from Compiler.circuit import Circuit

class FuseModule:
    """
    Use a FUSE circuit in a high-level program inside MP-SPDZ.
    Works similar to MP-SPDZ' Compiler.circuit module for Bristol Fashion circuits.
    """

    def __init__(self, path):
        self.path = path
        buf = open(self.path, 'rb').read()
        buf = bytearray(buf)
        self.module = mod.ModuleTable.GetRootAs(buf, 0)

    def __call__(self, *inputs):
        return self.run(*inputs)

    def run(self, *inputs):
        # get main circuit
        entry_circ_buf = self.module.Circuits(self.module.EntryPoint).CircuitBufferNestedRoot()
        entry_circ = circ.CircuitTable.GetRootAs(entry_circ_buf)
        # prepare inputs for circuit evaluation: map input node ID to its input value
        input_mappings = {}
        for i in range(entry_circ.InputsLength):
            input_mappings[entry_circ.Inputs(i)] = inputs[i]
        # evaluate the main circuit using the input map
        self.evaluate_circuit(entry_circ, input_mappings)
        # then extract the output values from that dict, as it was modified by the evaluate call
        outputs = []
        for i in range(entry_circ.OutputsLength):
            outputs.append(input_mappings[entry_circ.Outputs(i)])
        return tuple(outputs)


    def evaluate_circuit(self, circuit:circ.CircuitTable, input_mappings):
        # iterate over nodes and call evaluate
        for i in range(circuit.NodesLength):
            curr_node = circuit.Nodes(i)
            op = curr_node.Operation()
            # Input nodes: check if the value for this input exists
            if op == primop.Input:
                assert curr_node.Id() in input_mappings, f"Missing input value for node with ID: {curr_node.Id()}"
                continue
            # Output nodes: set output value from its input and continue
            elif op == primop.Output:
                assert curr_node.InputIdentifiersLength == 1 and curr_node.InputIdentifiers[0] in input_mappings
                input_mappings[curr_node.Id()] = input_mappings[curr_node.InputIdentifiers[0]]
                continue
            # Constant nodes: retrieve the constant value from the flexbuffer payload
            elif op == primop.Constant:
                # copy payload to bytearray for flexbuffers to use
                payload_bytearray = bytearray(curr_node.PayloadLength)
                for it in range(curr_node.PayloadLength):
                    payload_bytearray[it] = curr_node.Payload(it)
                constant_ref = flexbuffers.GetRoot(payload_bytearray)
                assert curr_node.OutputDatatypesLength == 1, "Constant node does not define its type!"
                constant_dt = curr_node.OutputDatatypes(0)
                # evaluate constant here, set in dict and continue
                constant = self.evaluate_constant(constant_ref, constant_dt)
                input_mappings[curr_node.Id()] = constant
                continue
            # All other nodes need input values: compute them here, then call evaluate to compute the node's output
            input_vals = []
            if curr_node.InputOffsetsIsNone:
                for in_node_it in range(curr_node.InputIdentifiersLength()):
                    in_node = curr_node.InputIdentifiers(in_node_it)
                    input_vals.append(input_mappings[in_node])
            else:
                assert curr_node.InputIdentifiersLength() == curr_node.InputOffsetsLength()
                for in_node_it in range(curr_node.InputIdentifiersLength()):
                    in_node = curr_node.InputIdentifiers(in_node_it)
                    offset = curr_node.InputOffsets(in_node_it)
                    value = input_mappings[in_node]
                    if type(value) in (list, tuple):
                        input_mappings[in_node] = value[offset]
                    else:
                        input_mappings[in_node] = value


            ret = self.evaluate_node(curr_node, input_vals)
            input_mappings[curr_node.Id()] = ret


    def evaluate_constant(self, constant:flexbuffers.Ref, type: DataTypeTable):
        pt = type.PrimitiveType()
        shape = type.Shape()
        # scalar value
        if (type.ShapeIsNone() or (type.Shape(0) == 0)):
            return self.evaluate_single_value(constant, pt)
        if (type.ShapeLength() == 1):
            res = []
            vec = constant.AsVector()
            for it in type.Shape(0):
                res.append(self.evaluate_single_value(vec[it], pt))
            return res
        else:
            raise NotImplementedError(f'Unsupported Constant Shape: {shape}')

    def evaluate_single_value(self, constant:flexbuffers.Ref, primitive_type:primtype):
        if (primitive_type == primtype.Bool):
            return constant.AsBool()
        elif (primitive_type == primitive_type.Float
                or primitive_type == primitive_type.Double):
            return constant.AsFloat()
        else:
            return constant.AsInt()

    def evaluate_node(self, node:NodeTable, input_vals):
        """Applies operation defined in node and returns the result."""
        assert len(input_vals) > 0, "Input values list must not be empty!"
        first, rest = input_vals[0], input_vals[1:]
        # Boolean Operations
        if node.Operation() == primop.Xor:
            return reduce(lambda x, y: x ^ y, input_vals)
        elif node.Operation() == primop.And:
            return reduce(lambda x, y: x & y, input_vals)
        elif node.Operation() == primop.Not:
            assert len(input_vals) == 1 , f"Operator {node.Operation()} expects only one argument, given: {len(input_vals)}"
            return not input_vals[0]
        elif node.Operation() == primop.Or:
            return reduce(lambda x, y: x | y, input_vals)
        elif node.Operation() == primop.Nand:
            and_res = reduce(lambda x, y: x & y, input_vals)
            return not and_res
        elif node.Operation() == primop.Nor:
            or_res = reduce(lambda x, y: x | y, input_vals)
            return not or_res
        elif node.Operation() == primop.Xnor:
            xor_res = reduce(lambda x, y: x ^ y, input_vals)
            return not xor_res
        # Comparisons
        elif node.Operation() == primop.Gt:
            assert len(input_vals) == 2 , f"Operator {node.Operation()} expects only two arguments, given: {len(input_vals)}"
            return input_vals[0] > input_vals[1]
        elif node.Operation() == primop.Ge:
            assert len(input_vals) == 2 , f"Operator {node.Operation()} expects only two arguments, given: {len(input_vals)}"
            return input_vals[0] >= input_vals[1]
        elif node.Operation() == primop.Lt:
            assert len(input_vals) == 2 , f"Operator {node.Operation()} expects only two arguments, given: {len(input_vals)}"
            return input_vals[0] < input_vals[1]
        elif node.Operation() == primop.Le:
            assert len(input_vals) == 2 , f"Operator {node.Operation()} expects only two arguments, given: {len(input_vals)}"
            return input_vals[0] <= input_vals[1]
        elif node.Operation() == primop.Eq:
            assert len(input_vals) == 2 , f"Operator {node.Operation()} expects only two arguments, given: {len(input_vals)}"
            return input_vals[0] == input_vals[1]
        # Arithmetic
        elif node.Operation() == primop.Add:
            return reduce(lambda x, y: x + y, input_vals)
        elif node.Operation() == primop.Mul:
            return reduce(lambda x, y: x * y, input_vals)
        elif node.Operation() == primop.Div:
            assert len(input_vals) == 2 , f"Operator {node.Operation()} expects only two arguments, given: {len(input_vals)}"
            return input_vals[0] / input_vals[1]
        elif node.Operation() == primop.Neg:
            assert len(input_vals) == 1 , f"Operator {node.Operation()} expects only one argument, given: {len(input_vals)}"
            return -input_vals[0]
        elif node.Operation() == primop.Sub:
            return reduce(lambda x, y: x - y, input_vals)
        # Control: call and mux
        elif node.Operation() == primop.Mux:
            assert len(input_vals) == 3 , f"Operator {node.Operation()} expects three arguments, given: {len(input_vals)}"
            return input_vals[0].if_else(input_vals[1], input_vals[2])
        elif node.Operation() == primop.CallSubcircuit:
            # get circuit from module
            subcircuit = self.module.Circuits(node.SubcircuitName()).CircuitBufferNestedRoot()
            # set input mappings for evaluation
            assert node.InputIdentifiersLength() == subcircuit.InputsLength(), f"call node and callee define different number of inputs: {node.InputIdentifiersLength()} != {subcircuit.InputsLength()}"
            input_mappings = {}
            for i in range(subcircuit.InputsLength()):
                input_mappings[subcircuit.Inputs(i)] = input_vals[i]
            # return outputs for this node from evaluation result
            ret = self.evaluate_circuit(subcircuit, input_mappings)
            # return value directly instead of a tuple with one element
            if len(ret) == 1:
                return ret[0]
            else:
                return ret
        # Split, Merge
        elif node.Operation() == primop.Split:
            raise NotImplementedError(f'Unsupported Operation: {node.Operation()}')
        elif node.Operation() == primop.Merge:
            raise NotImplementedError(f'Unsupported Operation: {node.Operation()}')
        else:
            raise NotImplementedError(f'Unsupported Operation: {node.Operation()}')
        # unreachable


circuit = ''

if __name__ == "__main__":
    print("Hello")
