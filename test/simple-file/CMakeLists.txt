add_executable(simple-file main.cpp)
target_include_directories(simple-file PRIVATE "${CMAKE_SOURCE_DIR}/include")
CppReflect(simple-file)
