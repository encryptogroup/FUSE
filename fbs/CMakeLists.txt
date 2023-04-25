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

# Inspired by https://github.com/encryptogroup/MOTION/blob/master/fbs/CMakeLists.txt

set(FBS_NAMES module.fbs circuit.fbs node.fbs datatype.fbs)

set(GENERATED_FILES "")

# generate commands that generate the header file for each flatbuffers file
foreach (FILE ${FBS_NAMES})
    set(THIS_FBS "${FILE}")
    string(REGEX MATCH "[A-Za-z0-9_]+\\.fbs" THIS_H ${THIS_FBS})
    string(REGEX MATCH "^[A-Za-z0-9_]+" THIS_H ${THIS_H})

    set(THIS_H "${THIS_H}_generated.h")
    if ((NOT ${FUSE_FBS_INCLUDE_PREFIX}/${THIS_H}) OR (${THIS_FBS} IS_NEWER_THAN ${FUSE_FBS_INCLUDE_PREFIX}/${THIS_H}))
        add_custom_command(OUTPUT "${FUSE_FBS_INCLUDE_PREFIX}/${THIS_H}"
#                COMMAND ${FLATBUFFERS_FLATC_EXECUTABLE} --cpp --scoped-enums --gen-object-api --gen-mutable --cpp-ptr-type std::shared_ptr -o ${FUSE_FBS_INCLUDE_PREFIX} ${FUSE_ROOT_DIR}/fbs/${THIS_FBS}
                COMMAND ${FLATBUFFERS_FLATC_EXECUTABLE} --cpp --python --scoped-enums --gen-object-api -o ${FUSE_FBS_INCLUDE_PREFIX} ${FUSE_ROOT_DIR}/fbs/${THIS_FBS}
                DEPENDS ${THIS_FBS})
    endif ()
    list(APPEND GENERATED_FILES "${FUSE_FBS_INCLUDE_PREFIX}/${THIS_H}")
endforeach ()

# check if flatc is already there
if(Flatbuffers_FOUND)
    add_custom_target(FUSE-fbs-headers-generation ALL DEPENDS ${GENERATED_FILES})
else()
    add_custom_target(FUSE-fbs-headers-generation ALL DEPENDS flatc ${GENERATED_FILES})
endif()
