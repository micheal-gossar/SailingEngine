#ifndef COMMON_H
#define COMMON_H

#include <array>
#include <cstdint>
inline constexpr char GAME_NAME[] = "Oblong Sailing Game";
inline constexpr char ENGINE_NAME[] = "Oblong Engine";

constexpr uint32_t WIDTH = 1280;
constexpr uint32_t HEIGHT = 720;

inline constexpr std::array validationLayers = {"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
constexpr bool inDebug = false;
#else
constexpr bool enableValidationLayers = true;
constexpr bool inDebug = true;
#endif
constexpr int MAX_FRAMES_IN_FLIGHT = 2;


#endif
