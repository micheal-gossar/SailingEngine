cmake_minimum_required(VERSION 3.25)

project(
    SailingEngine
    VERSION 0.1.0
    DESCRIPTION "Vulkan sailing game engine"
    LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(GNUInstallDirs)
include(CompilerWarnings)
include(SlangShaders)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build configuration" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    Debug Release RelWithDebInfo MinSizeRel
)

option(SAILING_WARNINGS_AS_ERRORS "Treat first-party warnings as errors" OFF)
option(SAILING_ENABLE_IPO "Enable interprocedural optimization in optimized builds" OFF)
option(SAILING_ENABLE_SANITIZERS "Enable address and undefined-behavior sanitizers" OFF)
option(SAILING_ENABLE_UNITY_BUILD "Enable CMake unity builds for first-party code" OFF)

find_package(Vulkan REQUIRED)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/glfw)

set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/glm)

add_library(sailing_imgui STATIC
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_glfw.cpp
    third_party/imgui/backends/imgui_impl_vulkan.cpp
)
add_library(SailingEngine::imgui ALIAS sailing_imgui)
target_include_directories(sailing_imgui SYSTEM PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui>"
)
target_link_libraries(sailing_imgui PUBLIC Vulkan::Vulkan glfw)
target_compile_definitions(sailing_imgui PUBLIC GLFW_INCLUDE_NONE)
target_compile_features(sailing_imgui PUBLIC cxx_std_20)

add_library(sailing_implot STATIC
    third_party/implot/implot.cpp
    third_party/implot/implot_items.cpp
)
add_library(SailingEngine::implot ALIAS sailing_implot)
target_include_directories(sailing_implot SYSTEM PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/implot>"
)
target_link_libraries(sailing_implot PUBLIC SailingEngine::imgui)
target_compile_features(sailing_implot PUBLIC cxx_std_20)

set(SAILING_SOURCES
    src/Debug/debugManager.cpp
    src/Logging/logger.cpp
    src/Renderer/renderer.cpp
    src/Window/window.cpp
    src/app.cpp
    src/main.cpp
)

set(SAILING_HEADERS
    include/SailingEngine/Debug/debugManager.h
    include/SailingEngine/Logging/logger.h
    include/SailingEngine/Renderer/renderer.h
    include/SailingEngine/Window/window.h
    include/SailingEngine/app.h
    include/SailingEngine/common.h
)

add_executable(oblong_sailing_game ${SAILING_SOURCES} ${SAILING_HEADERS})
add_executable(SailingEngine::game ALIAS oblong_sailing_game)

set_target_properties(oblong_sailing_game PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    UNITY_BUILD "${SAILING_ENABLE_UNITY_BUILD}"
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SAILING_SOURCES} ${SAILING_HEADERS})

target_include_directories(oblong_sailing_game PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)
target_link_libraries(oblong_sailing_game PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
    SailingEngine::imgui
    SailingEngine::implot
)
target_compile_definitions(oblong_sailing_game PRIVATE
    GLFW_INCLUDE_NONE
)

sailing_set_project_warnings(oblong_sailing_game)

if(SAILING_ENABLE_SANITIZERS)
    sailing_enable_sanitizers(oblong_sailing_game)
endif()

if(SAILING_ENABLE_IPO)
    include(CheckIPOSupported)
    check_ipo_supported(
        RESULT SAILING_IPO_SUPPORTED
        OUTPUT SAILING_IPO_ERROR
        LANGUAGES CXX
    )
    if(NOT SAILING_IPO_SUPPORTED)
        message(FATAL_ERROR "Interprocedural optimization is unavailable: ${SAILING_IPO_ERROR}")
    endif()
    set_property(TARGET oblong_sailing_game PROPERTY
        INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
        INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON
        INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL ON
    )
endif()

sailing_compile_slang(
    TARGET oblong_sailing_game
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/shader.slang"
    OUTPUT_NAME "slang.spv"
)

if(MINGW)
    target_link_options(oblong_sailing_game PRIVATE -static-libgcc -static-libstdc++)

    get_filename_component(MINGW_RUNTIME_DIRECTORY "${CMAKE_CXX_COMPILER}" DIRECTORY)
    add_custom_command(TARGET oblong_sailing_game POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${MINGW_RUNTIME_DIRECTORY}/libwinpthread-1.dll"
            "$<TARGET_FILE_DIR:oblong_sailing_game>"
        COMMENT "Deploying MinGW runtime"
        VERBATIM
    )
endif()

install(TARGETS oblong_sailing_game RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES "${SAILING_COMPILED_SHADER}"
    DESTINATION "${CMAKE_INSTALL_BINDIR}/assets/shaders"
)

message(STATUS "SailingEngine ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler:   ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Slang:      ${SLANGC_EXECUTABLE}")
