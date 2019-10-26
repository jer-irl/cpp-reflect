function(CppReflect TOOL_PATH TARGET)
    get_property(_global_compile_options GLOBAL PROPERTY COMPILE_OPTIONS)
    get_target_property(_target_compile_options ${TARGET} COMPILE_OPTIONS)
    get_target_property(_sources ${TARGET} SOURCES)

    foreach(_source ${_sources})
        get_filename_component(_source_name "${_source}" NAME)
        set(_ast_file_path "${CMAKE_BINARY_DIR}/reflect/${_source_name}.ast")
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

        add_custom_command(OUTPUT "${_ast_file_path}" "${_gen_file_path}"
            COMMAND "${TOOL_PATH}" -oast "${_ast_file_path}" -ocpp "${_gen_file_path}" "${_source}" -- ${_compile_flags}
            MAIN_DEPENDENCY "${_source}"
            DEPENDS cpp-reflect-codegen
            IMPLICIT_DEPENDS CXX "${_source}"
        )

        target_sources(${TARGET} PRIVATE "${_gen_file_path}")

        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND echo TODO
        )
    endforeach()

endfunction()
