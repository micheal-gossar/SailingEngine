cmake_minimum_required(VERSION 3.20)

project(oblong_sailing_game VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    set(PERFORMANCE_COMPILE_FLAGS "/O2 /Ob1 /DNDEBUG /Zi")
    set(PERFORMANCE_LINK_FLAGS "/DEBUG /INCREMENTAL:NO")
else()
    set(PERFORMANCE_COMPILE_FLAGS "-O2 -g -DNDEBUG")
    set(PERFORMANCE_LINK_FLAGS "")
endif()

set(CMAKE_C_FLAGS_PERFORMANCE "${PERFORMANCE_COMPILE_FLAGS}" CACHE STRING
    "C compiler flags for Performance builds" FORCE)
set(CMAKE_CXX_FLAGS_PERFORMANCE "${PERFORMANCE_COMPILE_FLAGS}" CACHE STRING
    "C++ compiler flags for Performance builds" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_PERFORMANCE "${PERFORMANCE_LINK_FLAGS}" CACHE STRING
    "Linker flags for Performance builds" FORCE)
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    Debug Release Performance RelWithDebInfo MinSizeRel
)

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

file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
)

file(GLOB_RECURSE GAME_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/header/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/header/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/header/*.hxx"
)

add_executable(oblong_sailing_game
    ${GAME_SOURCES}
    ${GAME_HEADERS}
)
target_include_directories(oblong_sailing_game PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/header"
    "${CMAKE_CURRENT_SOURCE_DIR}/header/Renderer"
    "${CMAKE_CURRENT_SOURCE_DIR}/header/Window"
)
target_link_libraries(oblong_sailing_game PRIVATE Vulkan::Vulkan glfw glm::glm)
target_compile_definitions(oblong_sailing_game PRIVATE
    GLFW_INCLUDE_NONE
    $<$<CONFIG:Performance>:PREFORMANCE_>
)

if(MINGW)
    target_link_options(oblong_sailing_game PRIVATE -static-libgcc -static-libstdc++)

    get_filename_component(MINGW_RUNTIME_DIRECTORY "${CMAKE_CXX_COMPILER}" DIRECTORY)
    add_custom_command(TARGET oblong_sailing_game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW_RUNTIME_DIRECTORY}/libwinpthread-1.dll"
            "$<TARGET_FILE_DIR:oblong_sailing_game>"
    )
endif()

if(MSVC)
    target_compile_options(oblong_sailing_game PRIVATE /W4 /permissive-)
else()
    target_compile_options(oblong_sailing_game PRIVATE -Wall -Wextra -Wpedantic)
endif()
