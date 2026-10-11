#include "Renderer/renderer.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

Renderer::Renderer() {
    InitVulkan();
}

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

    if (inDebug) {
        std::cout << "Created a Vulkan instance\n";
        std::cout << "Available extensions:\n";
        for (const auto& extension : extensionProperties) {
            std::cout << '\t' << extension.extensionName << '\n';
        }
        std::cout << std::flush;
    }
}

std::vector<const char*> getRequiredInstanceExtensions()
{
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    return extensions;
}