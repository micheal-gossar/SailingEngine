cmake_minimum_required(VERSION 3.20)

project(oblong_sailing_game VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/glfw)

set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/glm)

add_executable(oblong_sailing_game
    src/main.cpp
    src/app.cpp
    src/Window/window.cpp
    src/Renderer/renderer.cpp
    header/common.h
    header/app.h
    header/Window/window.h
    header/Renderer/renderer.h
)
target_include_directories(oblong_sailing_game PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/header"
    "${CMAKE_CURRENT_SOURCE_DIR}/header/Renderer"
    "${CMAKE_CURRENT_SOURCE_DIR}/header/Window"
)
target_link_libraries(oblong_sailing_game PRIVATE Vulkan::Vulkan glfw glm::glm)

if(MSVC)
    target_compile_options(oblong_sailing_game PRIVATE /W4 /permissive-)
else()
    target_compile_options(oblong_sailing_game PRIVATE -Wall -Wextra -Wpedantic)
endif()
