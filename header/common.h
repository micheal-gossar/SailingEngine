#ifndef COMMON_H
#define COMMON_H

#include <array>
#include <cstdint>

#define GAME_NAME "Oblong Sailing Game"
#define ENGINE_NAME "Oblong Engine"

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

#ifdef PREFORMANCE_
constexpr bool preformance = true;
#else
constexpr bool preformance = false;
#endif

#endif
