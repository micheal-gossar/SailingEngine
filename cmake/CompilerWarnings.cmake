function(sailing_set_project_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wshadow
        )
    endif()

    set_property(TARGET ${target} PROPERTY
        COMPILE_WARNING_AS_ERROR "${SAILING_WARNINGS_AS_ERRORS}"
    )
endfunction()

function(sailing_enable_sanitizers target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /fsanitize=address)
        target_link_options(${target} PRIVATE /fsanitize=address)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target} PRIVATE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
        )
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    else()
        message(FATAL_ERROR
            "Sanitizers are not configured for ${CMAKE_CXX_COMPILER_ID}"
        )
    endif()
endfunction()
