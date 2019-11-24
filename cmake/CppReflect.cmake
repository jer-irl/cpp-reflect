set(CppReflectASTRegistrationFileTemplate
"
#include <CppReflect/CppReflect.hpp>
extern const char @_ast_symbol_name@_start;
extern const char @_ast_symbol_name@_end;
static CppReflect::Details::ASTEntry @_ast_symbol_name@_entry{\"@_source_path@\", &@_ast_symbol_name@_start, &@_ast_symbol_name@_end};
"
)

set(CppReflectCompilationDatabaseFileTemplate
"
#include <CppReflect/CppReflect.hpp>
extern const char @_compilation_db_symbol_name@_start;
extern const char @_compilation_db_symbol_name@_end;
static CppReflect::Details::CompilationDatabaseEntry @_compilation_db_symbol_name@_entry{&@_compilation_db_symbol_name@_start, &@_compilation_db_symbol_name@_end};
"
)

function(CppReflect TARGET)
    target_include_directories(${TARGET} SYSTEM PRIVATE ${CLANG_INCLUDE_DIRS})
    target_link_libraries(${TARGET} PRIVATE clangTooling stdc++fs)

    get_target_property(_clang_executable_loc clang LOCATION)

    get_property(_global_compile_options GLOBAL PROPERTY COMPILE_OPTIONS)
    get_target_property(_target_compile_options ${TARGET} COMPILE_OPTIONS)
    get_target_property(_sources ${TARGET} SOURCES)

    foreach(_source ${_sources})
        get_filename_component(_source_name "${_source}" NAME)
        set(_source_path "${CMAKE_CURRENT_SOURCE_DIR}/${_source}")
        set(_ast_file_path "${CMAKE_BINARY_DIR}/reflect/${_source_name}.ast")
        set(_ast_object_file_path "${_ast_file_path}.o")
        set(_gen_file_path "${CMAKE_BINARY_DIR}/reflect/${_source_name}-generated.cpp")
        get_source_file_property(_source_compile_flags "${_source}" COMPILE_FLAGS)

        unset(_compile_flags)
        if(_global_compile_options)
            set(_compile_flags "${_compile_flags} ${_global_compile_options}")
        endif()
        if(_target_compile_options)
            set(_compile_flags "${_compile_flags} ${_target_compile_options}")
        endif()
        if(_source_compile_flags)
            set(_compile_flags "${_compile_flags} ${_source_compile_flags}")
        endif()

        get_target_property(_include_directories ${TARGET} INCLUDE_DIRECTORIES)

        add_custom_command(OUTPUT "${_ast_file_path}"
            COMMAND ${_clang_executable_loc} -emit-ast -o ${_ast_file_path} -std=c++17 ${_compile_flags} -I${LLVM_LIBRARY_DIR}/clang/8.0.0/include "-I$<JOIN:$<TARGET_PROPERTY:${TARGET},INCLUDE_DIRECTORIES>,;-I>" "${_source_path}"
            DEPENDS ${_source}
            IMPLICIT_DEPENDS CXX ${_source}
            COMMAND_EXPAND_LISTS
        )

        add_custom_command(OUTPUT "${_ast_object_file_path}"
            COMMAND ld -r -b binary -o ${_ast_object_file_path} ${_ast_file_path}
            DEPENDS ${_ast_file_path}
        )

        string(MAKE_C_IDENTIFIER ${_ast_file_path} _ast_symbol_name)
        set(_ast_symbol_name "_binary_${_ast_symbol_name}")
        string(CONFIGURE "${CppReflectASTRegistrationFileTemplate}" _generated_file_contents)
        file(WRITE ${_gen_file_path} "${_generated_file_contents}")

        target_sources(${TARGET} PRIVATE "${_gen_file_path}" "${_ast_object_file_path}")

        set_source_files_properties("${_ast_object_file_path}" PROPERTIES EXTERNAL_OBJECT true)

    endforeach()

    set(CMAKE_EXPORT_COMPILE_COMMANDS true)
    set(_compilation_db_path ${CMAKE_BINARY_DIR}/compile_commands.json)
    set(_compilation_db_object_path ${CMAKE_BINARY_DIR}/reflect/compile_commands.json.o)
    set(_compilation_db_registration_path ${CMAKE_BINARY_DIR}/reflect/compile_commands.json-generated.cpp)

    add_custom_command(OUTPUT ${_compilation_db_object_path}
        COMMAND ld -r -b binary -o ${_compilation_db_object_path} ${_compilation_db_path}
        DEPENDS ${_compilation_db_path}
    )

    string(MAKE_C_IDENTIFIER ${_compilation_db_path} _compilation_db_symbol_name)
    set(_compilation_db_symbol_name "_binary_${_compilation_db_symbol_name}")
    string(CONFIGURE "${CppReflectCompilationDatabaseFileTemplate}" _generated_file_contents)
    file(WRITE ${_compilation_db_registration_path} "${_generated_file_contents}")

    target_sources(${TARGET} PRIVATE "${_compilation_db_registration_path}" "${_compilation_db_object_path}")

    set_source_files_properties("${_compilation_db_object_path}" PROPERTIES EXTERNAL_OBJECT true)

endfunction()
