#include "Renderer/renderer.h"

Renderer::Renderer(Window* window)
    : window{window} {
   
    InitVulkan();
     
}

Renderer::~Renderer() = default;

void Renderer::InitVulkan() {
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
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

void Renderer::createSurface() {
    VkSurfaceKHR rawSurface = VK_NULL_HANDLE;
    if (glfwCreateWindowSurface(
            *instance,
            window->GetWindow(),
            nullptr,
            &rawSurface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface");
    }

    surface = vk::raii::SurfaceKHR(instance, rawSurface);
}

void Renderer::pickPhysicalDevice() {
      std::vector<vk::raii::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
  auto const devIter = std::ranges::find_if( physicalDevices, [&]( auto const & physicalDevice ) { return isDeviceSuitable( physicalDevice ); } );
  if ( devIter == physicalDevices.end() )
  {
    throw std::runtime_error( "failed to find a suitable GPU!" );
  }
  physicalDevice = *devIter;
  const auto properties = physicalDevice.getProperties();

  logger.Log("Picked Physical Device : " + std::string(properties.deviceName.data()), "Debug");
}

bool Renderer::isDeviceSuitable(vk::raii::PhysicalDevice const &physicalDevice) {
// Check if the physicalDevice supports the Vulkan 1.3 API version
  bool supportsVulkan1_3 = physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;

  // Check if any of the queue families support graphics operations
  auto queueFamilies    = physicalDevice.getQueueFamilyProperties();
  bool supportsGraphics = std::ranges::any_of( queueFamilies, []( auto const & qfp ) { return !!( qfp.queueFlags & vk::QueueFlagBits::eGraphics ); } );

  // Check if all required physicalDevice extensions are available
  auto availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
  bool supportsAllRequiredExtensions =
    std::ranges::all_of( requiredDeviceExtension,
                         [&availableDeviceExtensions]( auto const & requiredDeviceExtension )
                         {
                           return std::ranges::any_of( availableDeviceExtensions,
                                                       [requiredDeviceExtension]( auto const & availableDeviceExtension )
                                                       { return strcmp( availableDeviceExtension.extensionName, requiredDeviceExtension ) == 0; } );
                         } );

  // Check if the physicalDevice supports the required features (shader draw parameters, dynamic rendering and extended dynamic state)
  auto features                 = physicalDevice.template getFeatures2<vk::PhysicalDeviceFeatures2,
                                                                       vk::PhysicalDeviceVulkan11Features,
                                                                       vk::PhysicalDeviceVulkan13Features,
                                                                       vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
  bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
                                  features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
                                  features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

  // Return true if the physicalDevice meets all the criteria
  return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
}

void Renderer::createLogicalDevice() {
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

    uint32_t queueIndex = ~0u;
    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); ++qfpIndex) {
        if (static_cast<bool>(
                queueFamilyProperties[qfpIndex].queueFlags &
                vk::QueueFlagBits::eGraphics) &&
            physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface)) {
            queueIndex = qfpIndex;
            break;
        }
    }

    if (queueIndex == ~0u) {
        throw std::runtime_error(
            "Could not find a queue for graphics and present");
    }

    vk::StructureChain<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
        featureChain;
    featureChain
        .get<vk::PhysicalDeviceVulkan11Features>()
        .shaderDrawParameters = true;
    featureChain
        .get<vk::PhysicalDeviceVulkan13Features>()
        .dynamicRendering = true;
    featureChain
        .get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>()
        .extendedDynamicState = true;

    float queuePriority = 0.5f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
        {},
        queueIndex,
        1,
        &queuePriority
    };
    vk::DeviceCreateInfo deviceCreateInfo{
        {},
        1,
        &deviceQueueCreateInfo,
        0,
        nullptr,
        static_cast<uint32_t>(requiredDeviceExtension.size()),
        requiredDeviceExtension.data(),
        nullptr,
        &featureChain.get<vk::PhysicalDeviceFeatures2>()
    };

    device = vk::raii::Device(physicalDevice, deviceCreateInfo);
    graphicsQueue = vk::raii::Queue(device, queueIndex, 0);
    logger.Log("Created a Logical Device", "Debug");
}

std::vector<const char*> Renderer::getRequiredInstanceExtensions()
{
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    return extensions;
}