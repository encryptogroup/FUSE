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

add_library(FUSE
        core/IR.h
        core/IR.cpp
        core/util/IOHandlers.h
        core/util/IOHandlers.cpp
        core/ModuleBuilder.h
        core/ModuleBuilder.cpp
        core/ModuleWrapper.h
        core/ModuleWrapper.cpp
        core/BaseVisitor.h
        core/BaseVisitor.cpp
        core/PrimitiveTypeTraits.hpp
        core/PrimitiveOperationPolicies.hpp
        backend/PlaintextInterpreter.hpp
        backend/Evaluator.hpp
        backend/DOTBackend.cpp
        backend/BristolFormatBackend.cpp
        passes/DeadNodeEliminator.h
        passes/DeadNodeEliminator.cpp
        passes/NodeSuccessorsAnalysis.h
        passes/NodeSuccessorsAnalysis.cpp
        passes/ConstantFolder.h
        passes/ConstantFolder.cpp
        passes/FrequentSubcircuitReplacement.cpp
        passes/InstructionVectorization.cpp
        passes/DepthAnalysis.cpp
        util/ModuleGenerator.h
        util/ModuleGenerator.cpp
        )

target_include_directories(FUSE PUBLIC
        ${FUSE_SOURCE_DIR}/backend
        ${FUSE_SOURCE_DIR}/core
        ${FUSE_SOURCE_DIR}/core/util
        ${FUSE_SOURCE_DIR}/frontend
        ${FUSE_SOURCE_DIR}/passes
        ${FUSE_SOURCE_DIR}/util
        )

if (FUSE_USE_ADDRESS_SANITIZER)
    target_compile_options(FUSE PUBLIC "-fno-omit-frame-pointer" "-fsanitize-recover=address")
    target_link_options(FUSE PUBLIC "-fno-omit-frame-pointer" "-fsanitize=address")
endif ()

target_compile_features(FUSE PUBLIC cxx_std_20)

set_target_properties(FUSE
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        )
