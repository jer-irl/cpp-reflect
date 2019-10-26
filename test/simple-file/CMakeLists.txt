add_executable(simple-file main.cpp)
CppReflect("${CMAKE_BINARY_DIR}/tools/cpp-reflect-codegen/cpp-reflect-codegen" simple-file)
