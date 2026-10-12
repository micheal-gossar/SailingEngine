#include "Renderer/renderer.h"

#include "Debug/debugManager.h"
#include "common.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>

Renderer::Renderer() {
    InitVulkan();
}

Renderer::~Renderer() = default;

void Renderer::InitVulkan() {
    createInstance();
}

void Renderer::createInstance() {
    constexpr vk::ApplicationInfo appInfo{
        GAME_NAME,
        VK_MAKE_VERSION(1, 0, 0),
        ENGINE_NAME,
        VK_MAKE_VERSION(1, 0, 0),
        vk::ApiVersion13
    };

       // Get the required extensions.
    auto requiredExtensions = getRequiredInstanceExtensions();
       if (enableValidationLayers) {
           requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
       }

    // Check if the required extensions are supported by the Vulkan implementation.
    auto extensionProperties = context.enumerateInstanceExtensionProperties();
    auto unsupportedPropertyIt =
        std::ranges::find_if(requiredExtensions,
                                [&extensionProperties](auto const &requiredExtension) {
                                    return std::ranges::none_of(extensionProperties,
                                                                [requiredExtension](auto const &extensionProperty) { return strcmp(extensionProperty.extensionName, requiredExtension) == 0; });
                                });
    if (unsupportedPropertyIt != requiredExtensions.end())
    {
        throw std::runtime_error("Required extension not supported: " + std::string(*unsupportedPropertyIt));
    }

    std::vector<const char*> requiredLayers;
    if (enableValidationLayers) {
        requiredLayers.assign(validationLayers.begin(), validationLayers.end());
    }

    const auto layerProperties = context.enumerateInstanceLayerProperties();
    const auto unsupportedLayer = std::ranges::find_if(
        requiredLayers,
        [&layerProperties](const char* requiredLayer) {
            return std::ranges::none_of(
                layerProperties,
                [requiredLayer](const auto& layerProperty) {
                    return std::strcmp(layerProperty.layerName,
                                       requiredLayer) == 0;
                });
        });

    if (unsupportedLayer != requiredLayers.end()) {
        throw std::runtime_error(
            "Required Vulkan layer not supported: " +
            std::string(*unsupportedLayer));
    }

    const vk::InstanceCreateInfo createInfo{
        {},
        &appInfo,
        static_cast<uint32_t>(requiredLayers.size()),
        requiredLayers.data(),
        static_cast<uint32_t>(requiredExtensions.size()),
        requiredExtensions.data()
    };

    instance = vk::raii::Instance(context, createInfo);

#ifndef NDEBUG
    debugManager = std::make_unique<DebugManager>(logger, instance);
#endif

    logger.Log("Created a Vulkan instance", "Debug");
    logger.Log("Available extensions:", "Debug");

    for (const auto& extension : extensionProperties) {
        logger.Log(std::string{"\t"} + extension.extensionName.data(), "Debug");
    }
    logger.Flush();
}

std::vector<const char*> Renderer::getRequiredInstanceExtensions()
{
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    return extensions;
}