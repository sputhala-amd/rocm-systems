# MIT License
#
# Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
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

set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_SKIP_RPATH OFF)
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
set(CMAKE_CXX_VISIBILITY_PRESET "internal")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Threads REQUIRED)

add_library(rocprofiler-systems-attach-library SHARED)
add_library(rocprofiler-systems::rocprofiler-systems-attach-library ALIAS rocprofiler-systems-attach-library)


target_sources(
    rocprofiler-systems-attach-library
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/attach.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/ptrace_session.cpp)

target_include_directories(
    rocprofiler-systems-attach-library
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../>
           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(
    rocprofiler-systems-attach-library
    PUBLIC $<BUILD_INTERFACE:rocprofiler-systems::common-library>
           $<BUILD_INTERFACE:rocprofiler-systems::rocprofiler-systems-compile-definitions>
           $<BUILD_INTERFACE:Threads::Threads>
    )

add_target_cxx_flag_if_avail(rocprofiler-systems-attach-library "-ftls-model=global-dynamic")
add_target_cxx_flag_if_avail(rocprofiler-systems-attach-library "-g3")

set_target_properties(
    rocprofiler-systems-attach-library
    PROPERTIES OUTPUT_NAME ${BINARY_NAME_PREFIX}-attach
               VERSION ${PROJECT_VERSION}
               SOVERSION ${PROJECT_VERSION_MAJOR}
               BUILD_RPATH "\$ORIGIN"
               INSTALL_RPATH "\$ORIGIN")

rocprofiler_systems_strip_target(rocprofiler-systems-attach-library)

install(TARGETS rocprofiler-systems-attach-library DESTINATION ${CMAKE_INSTALL_LIBDIR})
