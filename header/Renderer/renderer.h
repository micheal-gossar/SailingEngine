#ifndef RENDERER_H
#define RENDERER_H

#include "Logging/logger.h"

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include <memory>
#include <vector>

class DebugManager;

class Renderer {
public:
    Renderer();
    ~Renderer();

    void InitVulkan();

private:
    void createInstance();
    Logger logger;
    std::vector<const char*> getRequiredInstanceExtensions();
    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;
#ifndef NDEBUG
    std::unique_ptr<DebugManager> debugManager;
#endif
};

#endif
