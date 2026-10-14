#include "SailingEngine/Renderer/renderer.h"

#include "SailingEngine/common.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>
#include <implot.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
void checkImGuiVulkanResult(VkResult result) {
    if (result < 0) {
        throw std::runtime_error("Dear ImGui Vulkan backend failed: " +
                                 vk::to_string(static_cast<vk::Result>(result)));
    }
}
} // namespace

Renderer::Renderer(Logger& outputLogger, Window* appWindow)
    : logger{outputLogger},
      renderingLog{logger.CreateStream("Rendering")},
      vulkanLog{renderingLog.Child("Vulkan")},
      instanceLog{vulkanLog.Child("Instance")},
      surfaceLog{vulkanLog.Child("Surface")},
      deviceLog{vulkanLog.Child("Device")},
      swapChainLog{vulkanLog.Child("Swap Chain")},
      pipelineLog{vulkanLog.Child("Pipeline")},
      commandLog{vulkanLog.Child("Commands")},
      synchronizationLog{vulkanLog.Child("Synchronization")},
      uiLog{renderingLog.Child("UI")},
      window{appWindow} {
    renderingLog.Info("Starting renderer");
    InitVulkan();
    renderingLog.Info("Renderer initialized successfully");
    logger.Flush();
}

Renderer::~Renderer() {
    if (guiInitialized) {
        uiLog.Debug("Shutting down renderer UI");
        device.waitIdle();
        shutdownGui();
    }
    renderingLog.Info("Renderer shutdown complete");
    logger.Flush();
}

void Renderer::InitVulkan() {
    vulkanLog.Debug("Starting Vulkan initialization");
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createGraphicsPipeline();
    createCommandPool();
    createCommandBuffer();
    createSyncObjects();
    initializeGui();
    vulkanLog.Debug("Vulkan initialization complete");
    renderingLog.Info("Vulkan renderer is ready");
}

void Renderer::createInstance() {
    instanceLog.Debug("Creating Vulkan instance");
    constexpr vk::ApplicationInfo appInfo{GAME_NAME, VK_MAKE_VERSION(1, 0, 0), ENGINE_NAME,
                                          VK_MAKE_VERSION(1, 0, 0), vk::ApiVersion13};

    // Get the required extensions.
    auto requiredExtensions = getRequiredInstanceExtensions();
    if (enableValidationLayers) {
        requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    instanceLog.Debug("Required instance extensions: " +
                      std::to_string(requiredExtensions.size()));

    // Check if the required extensions are supported by the Vulkan implementation.
    auto extensionProperties = context.enumerateInstanceExtensionProperties();
    auto unsupportedPropertyIt = std::ranges::find_if(
        requiredExtensions, [&extensionProperties](auto const& requiredExtension) {
            return std::ranges::none_of(
                extensionProperties, [requiredExtension](auto const& extensionProperty) {
                    return strcmp(extensionProperty.extensionName, requiredExtension) == 0;
                });
        });
    if (unsupportedPropertyIt != requiredExtensions.end()) {
        instanceLog.Error("Required extension is unavailable: " +
                          std::string(*unsupportedPropertyIt));
        throw std::runtime_error("Required extension not supported: " +
                                 std::string(*unsupportedPropertyIt));
    }

    std::vector<const char*> requiredLayers;
    if (enableValidationLayers) {
        requiredLayers.assign(validationLayers.begin(), validationLayers.end());
    }
    instanceLog.Debug("Validation layers " +
                      std::string(enableValidationLayers ? "enabled" : "disabled"));

    const auto layerProperties = context.enumerateInstanceLayerProperties();
    const auto unsupportedLayer =
        std::ranges::find_if(requiredLayers, [&layerProperties](const char* requiredLayer) {
            return std::ranges::none_of(
                layerProperties, [requiredLayer](const auto& layerProperty) {
                    return std::strcmp(layerProperty.layerName, requiredLayer) == 0;
                });
        });

    if (unsupportedLayer != requiredLayers.end()) {
        instanceLog.Error("Required validation layer is unavailable: " +
                          std::string(*unsupportedLayer));
        throw std::runtime_error("Required Vulkan layer not supported: " +
                                 std::string(*unsupportedLayer));
    }

    const vk::InstanceCreateInfo createInfo{{},
                                            &appInfo,
                                            static_cast<uint32_t>(requiredLayers.size()),
                                            requiredLayers.data(),
                                            static_cast<uint32_t>(requiredExtensions.size()),
                                            requiredExtensions.data()};

    instance = vk::raii::Instance(context, createInfo);

#ifndef NDEBUG
    debugManager = std::make_unique<DebugManager>(logger, instance);
#endif

    instanceLog.Debug("Created a Vulkan instance");
    instanceLog.Debug("Available extensions:");

    for (const auto& extension : extensionProperties) {
        instanceLog.Debug(extension.extensionName.data());
    }
    logger.Flush();
}

void Renderer::createSurface() {
    surfaceLog.Debug("Creating window surface");
    VkSurfaceKHR rawSurface = VK_NULL_HANDLE;
    if (glfwCreateWindowSurface(*instance, window->GetWindow(), nullptr, &rawSurface) !=
        VK_SUCCESS) {
        surfaceLog.Error("GLFW failed to create a Vulkan window surface");
        throw std::runtime_error("Failed to create window surface");
    }

    surface = vk::raii::SurfaceKHR(instance, rawSurface);
    surfaceLog.Debug("Created window surface");
}

void Renderer::pickPhysicalDevice() {
    deviceLog.Debug("Enumerating physical devices");
    std::vector<vk::raii::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
    deviceLog.Debug("Found " + std::to_string(physicalDevices.size()) +
                    " physical device(s)");
    auto const devIter = std::ranges::find_if(physicalDevices, [&](const auto& candidateDevice) {
        return isDeviceSuitable(candidateDevice);
    });
    if (devIter == physicalDevices.end()) {
        deviceLog.Error("No physical device satisfies the renderer requirements");
        throw std::runtime_error("failed to find a suitable GPU!");
    }
    physicalDevice = *devIter;
    const auto properties = physicalDevice.getProperties();

    deviceLog.Debug("Picked physical device: " + std::string(properties.deviceName.data()));
    deviceLog.Info("Using GPU: " + std::string(properties.deviceName.data()));
}

bool Renderer::isDeviceSuitable(const vk::raii::PhysicalDevice& candidateDevice) {
    const auto properties = candidateDevice.getProperties();
    // Check if the physicalDevice supports the Vulkan 1.3 API version
    bool supportsVulkan1_3 = properties.apiVersion >= vk::ApiVersion13;

    const auto queueFamilies = candidateDevice.getQueueFamilyProperties();
    bool supportsGraphicsAndPresent = false;
    for (uint32_t index = 0; index < queueFamilies.size(); ++index) {
        if (static_cast<bool>(queueFamilies[index].queueFlags & vk::QueueFlagBits::eGraphics) &&
            candidateDevice.getSurfaceSupportKHR(index, *surface)) {
            supportsGraphicsAndPresent = true;
            break;
        }
    }

    // Check if all required physicalDevice extensions are available
    auto availableDeviceExtensions = candidateDevice.enumerateDeviceExtensionProperties();
    bool supportsAllRequiredExtensions = std::ranges::all_of(
        requiredDeviceExtension, [&availableDeviceExtensions](const auto& requiredExtension) {
            return std::ranges::any_of(availableDeviceExtensions,
                                       [requiredExtension](const auto& availableDeviceExtension) {
                                           return strcmp(availableDeviceExtension.extensionName,
                                                         requiredExtension) == 0;
                                       });
        });

    // Check if the physicalDevice supports the required features.
    auto features = candidateDevice.template getFeatures2<
        vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
    const auto& vulkan13Features =
        features.template get<vk::PhysicalDeviceVulkan13Features>();
    bool supportsRequiredFeatures =
        features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
        vulkan13Features.dynamicRendering && vulkan13Features.synchronization2 &&
        features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>()
            .extendedDynamicState;

    deviceLog.Debug("Device '" + std::string(properties.deviceName.data()) +
                    "' suitability: Vulkan 1.3=" + (supportsVulkan1_3 ? "yes" : "no") +
                    ", graphics/present queue=" +
                    (supportsGraphicsAndPresent ? "yes" : "no") +
                    ", extensions=" + (supportsAllRequiredExtensions ? "yes" : "no") +
                    ", required features=" + (supportsRequiredFeatures ? "yes" : "no"));

    // Return true if the physicalDevice meets all the criteria
    return supportsVulkan1_3 && supportsGraphicsAndPresent && supportsAllRequiredExtensions &&
           supportsRequiredFeatures;
}

void Renderer::createLogicalDevice() {
    deviceLog.Debug("Creating logical device");
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
        physicalDevice.getQueueFamilyProperties();
    deviceLog.Debug("Inspecting " + std::to_string(queueFamilyProperties.size()) +
                    " queue family(ies)");

    queueIndex = ~0u;
    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); ++qfpIndex) {
        if (static_cast<bool>(queueFamilyProperties[qfpIndex].queueFlags &
                              vk::QueueFlagBits::eGraphics) &&
            physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface)) {
            queueIndex = qfpIndex;
            break;
        }
    }

    if (queueIndex == ~0u) {
        deviceLog.Error("No queue family supports both graphics and presentation");
        throw std::runtime_error("Could not find a queue for graphics and present");
    }
    deviceLog.Debug("Selected queue family " + std::to_string(queueIndex) +
                    " for graphics and presentation");

    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features,
                       vk::PhysicalDeviceVulkan13Features,
                       vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
        featureChain;
    featureChain.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters = true;
    featureChain.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering = true;
    featureChain.get<vk::PhysicalDeviceVulkan13Features>().synchronization2 = true;
    featureChain.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState =
        true;

    float queuePriority = 0.5f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{{}, queueIndex, 1, &queuePriority};
    vk::DeviceCreateInfo deviceCreateInfo{{},
                                          1,
                                          &deviceQueueCreateInfo,
                                          0,
                                          nullptr,
                                          static_cast<uint32_t>(requiredDeviceExtension.size()),
                                          requiredDeviceExtension.data(),
                                          nullptr,
                                          &featureChain.get<vk::PhysicalDeviceFeatures2>()};

    device = vk::raii::Device(physicalDevice, deviceCreateInfo);
    graphicsQueue = vk::raii::Queue(device, queueIndex, 0);
    deviceLog.Debug("Created logical device with dynamic rendering, synchronization2, and "
                    "extended dynamic state enabled");
}

void Renderer::createSwapChain() {
    swapChainLog.Debug("Creating swap chain");
    const vk::SurfaceCapabilitiesKHR surfaceCapabilities =
        physicalDevice.getSurfaceCapabilitiesKHR(*surface);
    const std::vector<vk::SurfaceFormatKHR> availableFormats =
        physicalDevice.getSurfaceFormatsKHR(*surface);
    const std::vector<vk::PresentModeKHR> availablePresentModes =
        physicalDevice.getSurfacePresentModesKHR(*surface);

    swapChainExtent = chooseSwapExtent(surfaceCapabilities);
    swapChainSurfaceFormat = chooseSwapSurfaceFormat(availableFormats);
    activePresentMode = chooseSwapPresentMode(availablePresentModes);
    const uint32_t minImageCount = chooseSwapMinImageCount(surfaceCapabilities);

    const vk::SwapchainCreateInfoKHR swapChainCreateInfo =
        vk::SwapchainCreateInfoKHR{}
            .setSurface(*surface)
            .setMinImageCount(minImageCount)
            .setImageFormat(swapChainSurfaceFormat.format)
            .setImageColorSpace(swapChainSurfaceFormat.colorSpace)
            .setImageExtent(swapChainExtent)
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
            .setImageSharingMode(vk::SharingMode::eExclusive)
            .setPreTransform(surfaceCapabilities.currentTransform)
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .setPresentMode(activePresentMode)
            .setClipped(true);

    swapChain = vk::raii::SwapchainKHR(device, swapChainCreateInfo);
    swapChainImages = swapChain.getImages();

    swapChainLog.Debug("Created swap chain: " + std::to_string(swapChainExtent.width) + "x" +
                       std::to_string(swapChainExtent.height) + ", " +
                       std::to_string(swapChainImages.size()) + " image(s), format " +
                       vk::to_string(swapChainSurfaceFormat.format) + ", present mode " +
                       vk::to_string(activePresentMode));
}

void Renderer::createImageViews() {
    swapChainLog.Debug("Creating swap chain image views");
    assert(swapChainImageViews.empty());

    const vk::ComponentMapping componentMapping{
        vk::ComponentSwizzle::eIdentity,
        vk::ComponentSwizzle::eIdentity,
        vk::ComponentSwizzle::eIdentity,
        vk::ComponentSwizzle::eIdentity,
    };
    const vk::ImageSubresourceRange subresourceRange{
        vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1,
    };

    swapChainImageViews.reserve(swapChainImages.size());
    for (const vk::Image image : swapChainImages) {
        const vk::ImageViewCreateInfo imageViewCreateInfo =
            vk::ImageViewCreateInfo{}
                .setImage(image)
                .setViewType(vk::ImageViewType::e2D)
                .setFormat(swapChainSurfaceFormat.format)
                .setComponents(componentMapping)
                .setSubresourceRange(subresourceRange);

        swapChainImageViews.emplace_back(device, imageViewCreateInfo);
    }

    swapChainLog.Debug("Created " + std::to_string(swapChainImageViews.size()) +
                       " swap chain image view(s)");
}

void Renderer::createGraphicsPipeline() {
    pipelineLog.Debug("Creating graphics pipeline");

    const std::vector<char> shaderCode = Logger::readFile("assets/shaders/slang.spv");
    pipelineLog.Debug("Loaded assets/shaders/slang.spv: " +
                      std::to_string(shaderCode.size()) + " bytes (" +
                      std::to_string(shaderCode.size() / sizeof(uint32_t)) + " words)");

    const vk::raii::ShaderModule shaderModule = createShaderModule(shaderCode);
    pipelineLog.Debug("Created shader module");

    const std::array shaderStages = {
        vk::PipelineShaderStageCreateInfo{}
            .setStage(vk::ShaderStageFlagBits::eVertex)
            .setModule(*shaderModule)
            .setPName("vertMain"),
        vk::PipelineShaderStageCreateInfo{}
            .setStage(vk::ShaderStageFlagBits::eFragment)
            .setModule(*shaderModule)
            .setPName("fragMain"),
    };
    pipelineLog.Debug("Configured " + std::to_string(shaderStages.size()) +
                      " shader stage(s) from a single module: vertMain (vertex), "
                      "fragMain (fragment)");

    constexpr std::array dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };
    const vk::PipelineDynamicStateCreateInfo dynamicState =
        vk::PipelineDynamicStateCreateInfo{}.setDynamicStates(dynamicStates);

    std::string dynamicStateNames;
    for (const vk::DynamicState state : dynamicStates) {
        if (!dynamicStateNames.empty()) {
            dynamicStateNames += ", ";
        }
        dynamicStateNames += vk::to_string(state);
    }
    pipelineLog.Debug("Dynamic state: " + dynamicStateNames);

    const vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
    const vk::PipelineInputAssemblyStateCreateInfo inputAssembly =
        vk::PipelineInputAssemblyStateCreateInfo{}
            .setTopology(vk::PrimitiveTopology::eTriangleList)
            .setPrimitiveRestartEnable(false);
    const vk::PipelineViewportStateCreateInfo viewportState =
        vk::PipelineViewportStateCreateInfo{}.setViewportCount(1).setScissorCount(1);
    const vk::PipelineRasterizationStateCreateInfo rasterizer =
        vk::PipelineRasterizationStateCreateInfo{}
            .setDepthClampEnable(false)
            .setRasterizerDiscardEnable(false)
            .setPolygonMode(vk::PolygonMode::eFill)
            .setCullMode(vk::CullModeFlagBits::eBack)
            .setFrontFace(vk::FrontFace::eClockwise)
            .setDepthBiasEnable(false)
            .setLineWidth(1.0F);
    const vk::PipelineMultisampleStateCreateInfo multisampling =
        vk::PipelineMultisampleStateCreateInfo{}
            .setRasterizationSamples(vk::SampleCountFlagBits::e1)
            .setSampleShadingEnable(false);

    pipelineLog.Debug("Fixed-function state: topology " +
                      vk::to_string(vk::PrimitiveTopology::eTriangleList) +
                      ", polygon mode " + vk::to_string(vk::PolygonMode::eFill) +
                      ", cull mode " + vk::to_string(vk::CullModeFlagBits::eBack) +
                      ", front face " + vk::to_string(vk::FrontFace::eClockwise) +
                      ", samples " + vk::to_string(vk::SampleCountFlagBits::e1));

    const vk::ColorComponentFlags colorComponents =
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    const vk::PipelineColorBlendAttachmentState colorBlendAttachment =
        vk::PipelineColorBlendAttachmentState{}.setBlendEnable(false).setColorWriteMask(
            colorComponents);
    const vk::PipelineColorBlendStateCreateInfo colorBlending =
        vk::PipelineColorBlendStateCreateInfo{}
            .setLogicOpEnable(false)
            .setLogicOp(vk::LogicOp::eCopy)
            .setAttachmentCount(1)
            .setPAttachments(&colorBlendAttachment);
    pipelineLog.Debug("Color blending disabled, write mask " +
                      vk::to_string(colorComponents) + ", 1 attachment");

    pipelineLayout = vk::raii::PipelineLayout(device, vk::PipelineLayoutCreateInfo{});
    pipelineLog.Debug("Created empty pipeline layout (0 descriptor sets, 0 push constant ranges)");

    const vk::Format colorAttachmentFormat = swapChainSurfaceFormat.format;
    const vk::PipelineRenderingCreateInfo renderingInfo =
        vk::PipelineRenderingCreateInfo{}.setColorAttachmentCount(1).setPColorAttachmentFormats(
            &colorAttachmentFormat);
    pipelineLog.Debug("Using dynamic rendering with color attachment format " +
                      vk::to_string(colorAttachmentFormat));

    const vk::GraphicsPipelineCreateInfo pipelineInfo = vk::GraphicsPipelineCreateInfo{}
                                                            .setPNext(&renderingInfo)
                                                            .setStages(shaderStages)
                                                            .setPVertexInputState(&vertexInputInfo)
                                                            .setPInputAssemblyState(&inputAssembly)
                                                            .setPViewportState(&viewportState)
                                                            .setPRasterizationState(&rasterizer)
                                                            .setPMultisampleState(&multisampling)
                                                            .setPColorBlendState(&colorBlending)
                                                            .setPDynamicState(&dynamicState)
                                                            .setLayout(*pipelineLayout)
                                                            .setRenderPass(nullptr);

    graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
    pipelineLog.Debug("Created graphics pipeline");
}

void Renderer::createCommandPool() {
    commandLog.Debug("Creating resettable command pool");
    const vk::CommandPoolCreateInfo poolInfo =
        vk::CommandPoolCreateInfo{}
            .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
            .setQueueFamilyIndex(queueIndex);

    commandPool = vk::raii::CommandPool(device, poolInfo);
    commandLog.Debug("Created command pool");
}

void Renderer::createCommandBuffer() {
    commandLog.Debug("Allocating primary command buffer");
    const vk::CommandBufferAllocateInfo allocInfo =
        vk::CommandBufferAllocateInfo{}
            .setCommandPool(*commandPool)
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandBufferCount(1);
    auto commandBuffers = vk::raii::CommandBuffers(device, allocInfo);
    commandBuffer = std::move(commandBuffers.front());
    commandLog.Debug("Allocated primary command buffer");
}

void Renderer::initializeGui() {
    uiLog.Debug("Creating Dear ImGui and ImPlot contexts");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    if (ImPlot::CreateContext() == nullptr) {
        uiLog.Error("ImPlot context creation failed");
        ImGui::DestroyContext();
        throw std::runtime_error("Failed to create the ImPlot context");
    }

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();

    if (!ImGui_ImplGlfw_InitForVulkan(window->GetWindow(), true)) {
        uiLog.Error("Dear ImGui GLFW backend initialization failed");
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
        throw std::runtime_error("Failed to initialize the Dear ImGui GLFW backend");
    }

    if (!initializeGuiVulkanBackend()) {
        uiLog.Error("Dear ImGui Vulkan backend initialization failed");
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
        throw std::runtime_error("Failed to initialize the Dear ImGui Vulkan backend");
    }

    guiInitialized = true;
    uiLog.Debug("Initialized Dear ImGui with docking and ImPlot");
    uiLog.Info("Engine UI initialized");
}

bool Renderer::initializeGuiVulkanBackend() {
    const VkFormat colorAttachmentFormat =
        static_cast<VkFormat>(swapChainSurfaceFormat.format);
    VkPipelineRenderingCreateInfo pipelineRenderingInfo{};
    pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingInfo.colorAttachmentCount = 1;
    pipelineRenderingInfo.pColorAttachmentFormats = &colorAttachmentFormat;

    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.ApiVersion = VK_API_VERSION_1_3;
    initInfo.Instance = static_cast<VkInstance>(*instance);
    initInfo.PhysicalDevice = static_cast<VkPhysicalDevice>(*physicalDevice);
    initInfo.Device = static_cast<VkDevice>(*device);
    initInfo.QueueFamily = queueIndex;
    initInfo.Queue = static_cast<VkQueue>(*graphicsQueue);
    initInfo.DescriptorPoolSize = 100;
    initInfo.MinImageCount = static_cast<uint32_t>(swapChainImages.size());
    initInfo.ImageCount = static_cast<uint32_t>(swapChainImages.size());
    initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = pipelineRenderingInfo;
    initInfo.UseDynamicRendering = true;
    initInfo.CheckVkResultFn = checkImGuiVulkanResult;
    initInfo.MinAllocationSize = 1024 * 1024;

    return ImGui_ImplVulkan_Init(&initInfo);
}

void Renderer::shutdownGui() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    guiInitialized = false;
}

void Renderer::buildGui() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

    static std::array<float, 240> frameTimes{};
    static std::size_t frameTimeSampleCount = 0;
    static float accumulatedFrameTime = 0.0F;
    if (frameTimeSampleCount == frameTimes.size()) {
        accumulatedFrameTime -= frameTimes.front();
    } else {
        ++frameTimeSampleCount;
    }
    std::rotate(frameTimes.begin(), frameTimes.begin() + 1, frameTimes.end());
    frameTimes.back() = ImGui::GetIO().DeltaTime * 1000.0F;
    accumulatedFrameTime += frameTimes.back();

    const float averageFrameTime =
        accumulatedFrameTime / static_cast<float>(frameTimeSampleCount);
    const float currentFps = frameTimes.back() > 0.0F ? 1000.0F / frameTimes.back() : 0.0F;
    const float averageFps = averageFrameTime > 0.0F ? 1000.0F / averageFrameTime : 0.0F;

    ImGui::SetNextWindowSize(ImVec2{420.0F, 300.0F}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Engine Performance")) {
        ImGui::Text("Dear ImGui %s", ImGui::GetVersion());
        ImGui::Text("Frame time: %.2f ms (average %.2f ms)", frameTimes.back(),
                    averageFrameTime);
        ImGui::Text("FPS: %.1f (average %.1f)", currentFps, averageFps);
        if (ImPlot::BeginPlot("Frame Time", ImVec2{-1.0F, 170.0F})) {
            ImPlot::SetupAxes("Frame", "Milliseconds");
            ImPlot::PlotLine("Frame Time", frameTimes.data(),
                             static_cast<int>(frameTimes.size()));
            ImPlot::EndPlot();
        }
    }
    ImGui::End();

    ImGui::SetNextWindowSize(ImVec2{320.0F, 140.0F}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Engine Settings")) {
        bool requestedVsync = vsyncEnabled;
        if (ImGui::Checkbox("VSync", &requestedVsync)) {
            vsyncEnabled = requestedVsync;
            swapChainRecreationRequested = true;
            uiLog.Info("VSync setting changed to " +
                       std::string(vsyncEnabled ? "enabled" : "disabled"));
            swapChainLog.Debug("Queued swap chain recreation for the VSync change");
        }

        const std::string presentModeName = vk::to_string(activePresentMode);
        ImGui::Text("Active present mode: %s", presentModeName.c_str());
        if (!vsyncEnabled && activePresentMode == vk::PresentModeKHR::eFifo) {
            ImGui::TextDisabled("VSync-off modes are unavailable on this device.");
        }
    }
    ImGui::End();
    ImGui::Render();
}

void Renderer::recreateSwapChain() {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window->GetWindow(), &width, &height);
    while ((width == 0 || height == 0) && !glfwWindowShouldClose(window->GetWindow())) {
        glfwWaitEvents();
        glfwGetFramebufferSize(window->GetWindow(), &width, &height);
    }
    if (glfwWindowShouldClose(window->GetWindow())) {
        return;
    }

    device.waitIdle();
    swapChainLog.Debug("GPU idle; rebuilding swap chain resources");
    commandBuffer.reset();
    graphicsPipeline = nullptr;
    pipelineLayout = nullptr;
    drawFence = nullptr;
    renderFinishedSemaphores.clear();
    presentCompleteSemaphore = nullptr;
    swapChainImageViews.clear();
    swapChainImages.clear();
    swapChain = nullptr;

    createSwapChain();
    createImageViews();
    createGraphicsPipeline();
    createSyncObjects();

    swapChainLog.Debug("Recreated swap chain with present mode " +
                       vk::to_string(activePresentMode));
    swapChainLog.Info("VSync " + std::string(vsyncEnabled ? "enabled" : "disabled") +
                      "; active present mode is " + vk::to_string(activePresentMode));
    logger.Flush();
}

void Renderer::recordCommandBuffer(uint32_t imageIndex) {
    commandBuffer.reset();
    commandBuffer.begin(vk::CommandBufferBeginInfo{});
    transitionImageLayout(imageIndex, vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eColorAttachmentOptimal, {},
                          vk::AccessFlagBits2::eColorAttachmentWrite,
                          vk::PipelineStageFlagBits2::eTopOfPipe,
                          vk::PipelineStageFlagBits2::eColorAttachmentOutput);

    const vk::ClearValue clearColor{
        vk::ClearColorValue{std::array{0.0F, 0.0F, 0.0F, 1.0F}}};
    const vk::RenderingAttachmentInfo attachmentInfo =
        vk::RenderingAttachmentInfo{}
            .setImageView(*swapChainImageViews[imageIndex])
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setClearValue(clearColor);
    const vk::Rect2D renderArea{vk::Offset2D{0, 0}, swapChainExtent};
    const vk::RenderingInfo renderingInfo =
        vk::RenderingInfo{}
            .setRenderArea(renderArea)
            .setLayerCount(1)
            .setColorAttachmentCount(1)
            .setPColorAttachments(&attachmentInfo);
    commandBuffer.beginRendering(renderingInfo);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
    const vk::Viewport viewport = vk::Viewport{}
                                      .setX(0.0F)
                                      .setY(0.0F)
                                      .setWidth(static_cast<float>(swapChainExtent.width))
                                      .setHeight(static_cast<float>(swapChainExtent.height))
                                      .setMinDepth(0.0F)
                                      .setMaxDepth(1.0F);
    commandBuffer.setViewport(0, viewport);
    commandBuffer.setScissor(0, renderArea);
    commandBuffer.draw(3, 1, 0, 0);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                    static_cast<VkCommandBuffer>(*commandBuffer));
    commandBuffer.endRendering();

    transitionImageLayout(imageIndex, vk::ImageLayout::eColorAttachmentOptimal,
                          vk::ImageLayout::ePresentSrcKHR,
                          vk::AccessFlagBits2::eColorAttachmentWrite, {},
                          vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                          vk::PipelineStageFlagBits2::eBottomOfPipe);
    commandBuffer.end();
}

void Renderer::transitionImageLayout(uint32_t imageIndex, vk::ImageLayout oldLayout,
                                     vk::ImageLayout newLayout,
                                     vk::AccessFlags2 sourceAccessMask,
                                     vk::AccessFlags2 destinationAccessMask,
                                     vk::PipelineStageFlags2 sourceStageMask,
                                     vk::PipelineStageFlags2 destinationStageMask) {
    const vk::ImageSubresourceRange subresourceRange =
        vk::ImageSubresourceRange{}
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseMipLevel(0)
            .setLevelCount(1)
            .setBaseArrayLayer(0)
            .setLayerCount(1);
    const vk::ImageMemoryBarrier2 barrier =
        vk::ImageMemoryBarrier2{}
            .setSrcStageMask(sourceStageMask)
            .setSrcAccessMask(sourceAccessMask)
            .setDstStageMask(destinationStageMask)
            .setDstAccessMask(destinationAccessMask)
            .setOldLayout(oldLayout)
            .setNewLayout(newLayout)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(swapChainImages[imageIndex])
            .setSubresourceRange(subresourceRange);
    const vk::DependencyInfo dependencyInfo =
        vk::DependencyInfo{}.setImageMemoryBarrierCount(1).setPImageMemoryBarriers(&barrier);
    commandBuffer.pipelineBarrier2(dependencyInfo);
}

void Renderer::Update() {
    if (swapChainRecreationRequested) {
        recreateSwapChain();
        swapChainRecreationRequested = false;
    }
    buildGui();
    DrawFrame();
}

void Renderer::WaitIdle() {
    device.waitIdle();
}

void Renderer::DrawFrame() {
    const auto fenceResult = device.waitForFences(*drawFence, vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess) {
        synchronizationLog.Error("Draw fence wait failed: " + vk::to_string(fenceResult));
        throw std::runtime_error("Failed to wait for the draw fence");
    }

    const auto [acquireResult, imageIndex] =
        swapChain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphore, nullptr);
    if (acquireResult != vk::Result::eSuccess &&
        acquireResult != vk::Result::eSuboptimalKHR) {
        swapChainLog.Error("Swap chain image acquisition failed: " +
                           vk::to_string(acquireResult));
        throw std::runtime_error("Failed to acquire a swap chain image: " +
                                 vk::to_string(acquireResult));
    }

    device.resetFences(*drawFence);
    recordCommandBuffer(imageIndex);

    const vk::PipelineStageFlags waitDestinationStageMask =
        vk::PipelineStageFlagBits::eColorAttachmentOutput;
    const vk::Semaphore waitSemaphore = *presentCompleteSemaphore;
    const vk::Semaphore signalSemaphore = *renderFinishedSemaphores.at(imageIndex);
    const vk::CommandBuffer submittedCommandBuffer = *commandBuffer;
    const vk::SubmitInfo submitInfo =
        vk::SubmitInfo{}
            .setWaitSemaphoreCount(1)
            .setPWaitSemaphores(&waitSemaphore)
            .setPWaitDstStageMask(&waitDestinationStageMask)
            .setCommandBufferCount(1)
            .setPCommandBuffers(&submittedCommandBuffer)
            .setSignalSemaphoreCount(1)
            .setPSignalSemaphores(&signalSemaphore);

    graphicsQueue.submit(submitInfo, *drawFence);

    const vk::SwapchainKHR presentedSwapChain = *swapChain;
    const vk::PresentInfoKHR presentInfo =
        vk::PresentInfoKHR{}
            .setWaitSemaphoreCount(1)
            .setPWaitSemaphores(&signalSemaphore)
            .setSwapchainCount(1)
            .setPSwapchains(&presentedSwapChain)
            .setPImageIndices(&imageIndex);
    const vk::Result presentResult = graphicsQueue.presentKHR(presentInfo);
    if (presentResult != vk::Result::eSuccess &&
        presentResult != vk::Result::eSuboptimalKHR) {
        swapChainLog.Error("Swap chain presentation failed: " +
                           vk::to_string(presentResult));
        throw std::runtime_error("Failed to present a swap chain image: " +
                                 vk::to_string(presentResult));
    }
}

void Renderer::createSyncObjects() {
    synchronizationLog.Debug("Creating frame synchronization objects");
    presentCompleteSemaphore = vk::raii::Semaphore(device, vk::SemaphoreCreateInfo{});
    renderFinishedSemaphores.clear();
    renderFinishedSemaphores.reserve(swapChainImages.size());
    for (std::size_t index = 0; index < swapChainImages.size(); ++index) {
        renderFinishedSemaphores.emplace_back(device, vk::SemaphoreCreateInfo{});
    }
    const vk::FenceCreateInfo fenceInfo =
        vk::FenceCreateInfo{}.setFlags(vk::FenceCreateFlagBits::eSignaled);
    drawFence = vk::raii::Fence(device, fenceInfo);
    synchronizationLog.Debug("Created frame synchronization objects for " +
                             std::to_string(swapChainImages.size()) + " swap chain images");
}

vk::raii::ShaderModule Renderer::createShaderModule(const std::vector<char>& code) const {
    if (code.empty() || (code.size() % sizeof(uint32_t)) != 0) {
        pipelineLog.Error("Shader byte code is empty or not aligned to 32-bit words");
        throw std::runtime_error("Shader byte code size is not a multiple of 4");
    }

    const vk::ShaderModuleCreateInfo shaderModuleCreateInfo =
        vk::ShaderModuleCreateInfo{}
            .setCodeSize(code.size())
            .setPCode(reinterpret_cast<const uint32_t*>(code.data()));

    return vk::raii::ShaderModule(device, shaderModuleCreateInfo);
}

vk::SurfaceFormatKHR
Renderer::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) const {
    assert(!availableFormats.empty());
    const auto formatIt = std::ranges::find_if(availableFormats, [](const auto& format) {
        return format.format == vk::Format::eB8G8R8A8Srgb &&
               format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
    });
    return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}

vk::PresentModeKHR Renderer::chooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR>& availablePresentModes) const {
    assert(std::ranges::any_of(availablePresentModes, [](auto presentMode) {
        return presentMode == vk::PresentModeKHR::eFifo;
    }));
    if (vsyncEnabled) {
        return vk::PresentModeKHR::eFifo;
    }

    for (const vk::PresentModeKHR preferredMode :
         {vk::PresentModeKHR::eImmediate, vk::PresentModeKHR::eMailbox}) {
        if (std::ranges::find(availablePresentModes, preferredMode) !=
            availablePresentModes.end()) {
            return preferredMode;
        }
    }
    return vk::PresentModeKHR::eFifo;
}
vk::Extent2D Renderer::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const {
    // currentExtent is only set to the special "undefined" value described above
    // when the window manager lets us choose the extent ourselves; any other value
    // means the surface already dictates a fixed extent that we must use as-is.
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window->GetWindow(), &width, &height);

    return {std::clamp<uint32_t>(width, capabilities.minImageExtent.width,
                                 capabilities.maxImageExtent.width),
            std::clamp<uint32_t>(height, capabilities.minImageExtent.height,
                                 capabilities.maxImageExtent.height)};
}

uint32_t
Renderer::chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR& surfaceCapabilities) const {
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    if ((0 < surfaceCapabilities.maxImageCount) &&
        (surfaceCapabilities.maxImageCount < minImageCount)) {
        minImageCount = surfaceCapabilities.maxImageCount;
    }
    return minImageCount;
}

std::vector<const char*> Renderer::getRequiredInstanceExtensions() {
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    if (glfwExtensions == nullptr) {
        instanceLog.Error("GLFW did not provide required Vulkan instance extensions");
        throw std::runtime_error("Failed to retrieve GLFW required extensions");
    }

    std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    instanceLog.Debug("GLFW requires " + std::to_string(glfwExtensionCount) +
                      " instance extension(s)");

    return extensions;
}