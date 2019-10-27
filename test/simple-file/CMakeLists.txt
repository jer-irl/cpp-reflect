add_executable(simple-file main.cpp)
target_include_directories(simple-file PRIVATE "${CMAKE_SOURCE_DIR}/include")
CppReflect("${CMAKE_BINARY_DIR}/tools/cpp-reflect-codegen/cpp-reflect-codegen" simple-file)

get_target_property(VAR simple-file SOURCES)
message(WARNING "${VAR}")
