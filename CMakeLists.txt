cmake_minimum_required(VERSION 3.10)
project(cpp-reflect)

find_package(Clang REQUIRED)
if(NOT Clang_FOUND)
    message(FATAL_ERROR "Clang libraries required")
endif()

add_subdirectory(tools)
