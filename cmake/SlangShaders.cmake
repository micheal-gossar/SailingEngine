set(SLANG_ROOT "" CACHE PATH "Path to the Slang SDK")

set(_SAILING_SLANG_HINTS)
if(SLANG_ROOT)
    list(APPEND _SAILING_SLANG_HINTS "${SLANG_ROOT}/bin")
endif()
if(DEFINED ENV{SLANG_ROOT})
    list(APPEND _SAILING_SLANG_HINTS "$ENV{SLANG_ROOT}/bin")
endif()
if(DEFINED ENV{VULKAN_SDK})
    list(APPEND _SAILING_SLANG_HINTS "$ENV{VULKAN_SDK}/Bin")
endif()

find_program(SLANGC_EXECUTABLE
    NAMES slangc
    HINTS ${_SAILING_SLANG_HINTS}
    DOC "Path to the Slang shader compiler"
    REQUIRED
)

function(sailing_compile_slang)
    set(options)
    set(one_value_args TARGET SOURCE OUTPUT_NAME)
    cmake_parse_arguments(SHADER "${options}" "${one_value_args}" "" ${ARGN})

    if(NOT SHADER_TARGET OR NOT TARGET "${SHADER_TARGET}")
        message(FATAL_ERROR "sailing_compile_slang requires an existing TARGET")
    endif()
    if(NOT SHADER_SOURCE OR NOT EXISTS "${SHADER_SOURCE}")
        message(FATAL_ERROR "Slang shader source does not exist: ${SHADER_SOURCE}")
    endif()
    if(NOT SHADER_OUTPUT_NAME)
        message(FATAL_ERROR "sailing_compile_slang requires OUTPUT_NAME")
    endif()

    set(output_directory "${CMAKE_CURRENT_BINARY_DIR}/generated/assets/shaders")
    set(output_file "${output_directory}/${SHADER_OUTPUT_NAME}")

    if(WIN32)
        set(compile_command
            "${CMAKE_COMMAND}" -E env "SLANGC=${SLANGC_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/shaderCompile.bat"
            "${SHADER_SOURCE}" "${output_file}"
        )
        set(compile_dependencies
            "${SHADER_SOURCE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/shaderCompile.bat"
        )
    else()
        set(compile_command
            "${SLANGC_EXECUTABLE}" "${SHADER_SOURCE}"
            -target spirv
            -profile spirv_1_4
            -emit-spirv-directly
            -fvk-use-entrypoint-name
            -entry vertMain
            -entry fragMain
            -o "${output_file}"
        )
        set(compile_dependencies "${SHADER_SOURCE}")
    endif()

    add_custom_command(
        OUTPUT "${output_file}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${output_directory}"
        COMMAND ${compile_command}
        DEPENDS ${compile_dependencies}
        COMMENT "Compiling ${SHADER_SOURCE}"
        VERBATIM
    )

    set(shader_target "${SHADER_TARGET}_shaders")
    add_custom_target("${shader_target}" ALL DEPENDS "${output_file}")
    add_dependencies("${SHADER_TARGET}" "${shader_target}")

    set_source_files_properties("${SHADER_SOURCE}" PROPERTIES HEADER_FILE_ONLY ON)
    target_sources("${SHADER_TARGET}" PRIVATE "${SHADER_SOURCE}")
    source_group("assets/shaders" FILES "${SHADER_SOURCE}")

    add_custom_command(TARGET "${SHADER_TARGET}" POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_FILE_DIR:${SHADER_TARGET}>/assets/shaders"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${output_file}"
            "$<TARGET_FILE_DIR:${SHADER_TARGET}>/assets/shaders/${SHADER_OUTPUT_NAME}"
        COMMENT "Deploying ${SHADER_OUTPUT_NAME}"
        VERBATIM
    )

    set(SAILING_COMPILED_SHADER "${output_file}" PARENT_SCOPE)
endfunction()
