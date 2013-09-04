cmake_minimum_required(VERSION 2.8)

set(LLVM_DIR "/home/romain/XStack/tools/llvm-3.3/install/share/llvm/cmake")
set(LLVM_COMPILER_IS_GCC_COMPATIBLE 1)
set(CMAKE_BUILD_TYPE "Debug")
find_package(LLVM)

include(AddLLVM)

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

add_llvm_loadable_module(OCRStatsPass OCRStatsPass.cpp)
