add_executable(cpp-reflect-codegen
    CppReflectCodegen.cpp
    CodeGeneration.cpp
)

target_include_directories(cpp-reflect-codegen PRIVATE ${CLANG_INCLUDE_DIRS})
target_link_libraries(cpp-reflect-codegen
    clangTooling
    )
