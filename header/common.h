#ifndef COMMON_H
#define COMMON_H

#define GLFW_INCLUDE_VULKAN

#define GAME_NAME "Oblong Sailing Game"
#define ENGINE_NAME "Oblong Engine"

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include <GLFW/glfw3.h>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

inline constexpr std::array validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
constexpr bool inDebug = false;
#else
constexpr bool enableValidationLayers = true;
constexpr bool inDebug = true;
#endif

#endif
