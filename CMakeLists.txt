cmake_minimum_required(VERSION 3.10)
project(cpp-reflect)

set(CMAKE_CXX_STANDARD 17)

find_package(LLVM REQUIRED)
if(NOT LLVM_FOUND)
    message(FATAL_ERROR "LLVM libraries required")
endif()
find_package(Clang REQUIRED COMPONENTS libTooling clang)
if(NOT Clang_FOUND)
    message(FATAL_ERROR "Clang libraries required")
endif()

include(cmake/CppReflect.cmake)

add_subdirectory(test)
