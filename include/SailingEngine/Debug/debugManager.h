#ifndef DEBUG_MANAGER_H
#define DEBUG_MANAGER_H

#include "SailingEngine/Logging/logger.h"

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

class DebugManager {
public:
    DebugManager(Logger& outputLogger, const vk::raii::Instance& instance);

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT* callbackData, void* userData);

private:
    LogStream validationLog;
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
};

#endif
