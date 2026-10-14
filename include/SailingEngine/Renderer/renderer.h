#ifndef RENDERER_H
#define RENDERER_H

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include "SailingEngine/Debug/debugManager.h"
#include "SailingEngine/Logging/logger.h"
#include "SailingEngine/Window/window.h"

#include <cstdint>
#include <memory>
#include <vector>

class Renderer {
public:
    Renderer(Logger& logger, Window* appWindow);
    ~Renderer();

    void InitVulkan();
    void Update();
    void WaitIdle();

private:
    void createInstance();
    void createSurface();
    void pickPhysicalDevice();
    bool isDeviceSuitable(const vk::raii::PhysicalDevice& candidateDevice);
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createGraphicsPipeline();
    void createCommandPool();
    void createCommandBuffer();
    void initializeGui();
    [[nodiscard]] bool initializeGuiVulkanBackend();
    void shutdownGui();
    void buildGui();
    void recreateSwapChain();
    void recordCommandBuffer(uint32_t imageIndex);
    void transitionImageLayout(uint32_t imageIndex, vk::ImageLayout oldLayout,
                               vk::ImageLayout newLayout, vk::AccessFlags2 sourceAccessMask,
                               vk::AccessFlags2 destinationAccessMask,
                               vk::PipelineStageFlags2 sourceStageMask,
                               vk::PipelineStageFlags2 destinationStageMask);
    void DrawFrame();
    void createSyncObjects();
    [[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const;
    [[nodiscard]] vk::SurfaceFormatKHR
    chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) const;
    [[nodiscard]] vk::PresentModeKHR
    chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) const;
    [[nodiscard]] vk::Extent2D
    chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const;
    [[nodiscard]] uint32_t
    chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR& surfaceCapabilities) const;
    [[nodiscard]] std::vector<const char*> getRequiredInstanceExtensions();

    Logger& logger;
    LogStream renderingLog;
    LogStream vulkanLog;
    LogStream instanceLog;
    LogStream surfaceLog;
    LogStream deviceLog;
    LogStream swapChainLog;
    LogStream pipelineLog;
    LogStream commandLog;
    LogStream synchronizationLog;
    LogStream uiLog;
    Window* window;
    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;
    vk::raii::SurfaceKHR surface = nullptr;
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    std::vector<const char*> requiredDeviceExtension = {vk::KHRSwapchainExtensionName};
    vk::raii::Device device = nullptr;
    uint32_t queueIndex = ~0u;
    vk::raii::Queue graphicsQueue = nullptr;
    vk::raii::SwapchainKHR swapChain = nullptr;
    std::vector<vk::Image> swapChainImages;
    vk::SurfaceFormatKHR swapChainSurfaceFormat;
    vk::Extent2D swapChainExtent;
    std::vector<vk::raii::ImageView> swapChainImageViews;
    vk::raii::PipelineLayout pipelineLayout = nullptr;
    vk::raii::Pipeline graphicsPipeline = nullptr;
    vk::raii::CommandPool commandPool = nullptr;
    vk::raii::CommandBuffer commandBuffer = nullptr;
    vk::raii::Semaphore presentCompleteSemaphore = nullptr;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    vk::raii::Fence drawFence = nullptr;
    vk::PresentModeKHR activePresentMode = vk::PresentModeKHR::eFifo;
    bool vsyncEnabled = true;
    bool swapChainRecreationRequested = false;
    bool guiInitialized = false;

#ifndef NDEBUG
    std::unique_ptr<DebugManager> debugManager;
#endif
};

#endif
