#ifndef RENDERER_H
#define RENDERER_H



#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif


#include "Debug/debugManager.h"
#include "common.h"


#include <algorithm>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include "Logging/logger.h"

#include "Window/window.h"
class Renderer {
public:
    Renderer(Window* window);
    ~Renderer();

    void InitVulkan();

private:
    void createInstance();
    void createSurface();
    void pickPhysicalDevice();
    bool isDeviceSuitable(vk::raii::PhysicalDevice const & physicalDevice);
    void createLogicalDevice();
    Window* window;
    Logger logger;
    std::vector<const char*> getRequiredInstanceExtensions();
    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;
    vk::raii::SurfaceKHR surface = nullptr;
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    std::vector<const char*> requiredDeviceExtension = {vk::KHRSwapchainExtensionName};
    vk::raii::Device device = nullptr;
    vk::raii::Queue graphicsQueue = nullptr;
    #ifndef NDEBUG
    std::unique_ptr<DebugManager> debugManager;
#endif
};

#endif
