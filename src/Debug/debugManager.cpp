#include "Debug/debugManager.h"

#include "common.h"

#include <cstdio>
#include <exception>
#include <string>

DebugManager::DebugManager(
    Logger& logger,
    const vk::raii::Instance& instance)
    : logger{logger}
{
    if (!enableValidationLayers) {
        return;
    }

    const vk::DebugUtilsMessageSeverityFlagsEXT severityFlags =
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    const vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags =
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
    const vk::DebugUtilsMessengerCreateInfoEXT createInfo{
        {},
        severityFlags,
        messageTypeFlags,
        &DebugManager::debugCallback,
        this
    };

    debugMessenger = instance.createDebugUtilsMessengerEXT(createInfo);
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugManager::debugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    vk::DebugUtilsMessageTypeFlagsEXT type,
    const vk::DebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData)
{
    auto* manager = static_cast<DebugManager*>(userData);
    if (manager == nullptr ||
        callbackData == nullptr ||
        callbackData->pMessage == nullptr) {
        return VK_FALSE;
    }

    if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning ||
        severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError) {
        try {
            manager->logger.Log(
                "Validation layer: severity " +
                    vk::to_string(severity) +
                    ", type " +
                    vk::to_string(type) +
                    ", message: " +
                    callbackData->pMessage,
                "Debug");
        }
        catch (const std::exception& exception) {
            std::fprintf(
                stderr,
                "Failed to log a Vulkan validation message: %s\n",
                exception.what());
        }
        catch (...) {
            std::fputs(
                "Failed to log a Vulkan validation message: unknown error\n",
                stderr);
        }
    }

    return VK_FALSE;
}
