#include "Vulkan/Pipeline.h"
#include "Utils/Logs.h"
#include <map>

#include "Utils/System.h"

const std::vector<char const*> ValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char *> RequiredDeviceExtension = {
    vk::KHRSwapchainExtensionName
};

Pipeline::~Pipeline()
{
    Deinit();
}

bool Pipeline::Init()
{
    if (!CreateInstance()) return false;
    if (!PickPhysicalDevice()) return false;
    if (!CreateWindow()) return false;
    CreateLogicalDevice();
    CreateSwapChain();
    return true;
}

void Pipeline::Deinit()
{
    if (Surface != VK_NULL_HANDLE)
    {
        SDL_Vulkan_DestroySurface(*Instance, Surface, NULL);
        Surface = VK_NULL_HANDLE;
    }

    if (Window != nullptr)
    {
        SDL_DestroyWindow(Window);
        Window = nullptr;
    }

    DebugMessenger.release();
    Extensions.clear(); Extensions.shrink_to_fit();
    Instance.release();
    PhysicalDevices.clear(); PhysicalDevices.shrink_to_fit();
    PhysicalDevice.release();
    if (SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)
    {
        SDL_Quit();
    }
}

void Pipeline::Pool()
{

}

bool Pipeline::CreateWindow()
{
    Window = SDL_CreateWindow(
        ENGINE_NAME,
        1024,
        576,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
    );

    if (!Window)
    {
        Logs::SdlError();
        return false;
    }

    if (!SDL_Vulkan_CreateSurface(Window, *Instance, NULL, &Surface))
    {
        Logs::SdlError();
        return false;
    }

    return true;
}

bool Pipeline::CreateInstance()
{
    /* Init SDL Video */
    if (SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)
    {
        Logs::Error("SDL video already initialized");
        return false;
    }

    if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
    {
        Logs::SdlError();
        return false;
    }

    AppInfo.pApplicationName = ENGINE_NAME;
    AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    AppInfo.pEngineName = "No Engine";
    AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    AppInfo.apiVersion = vk::ApiVersion14;

    // Get the required layers
    std::vector<char const*> requiredLayers;
    bool isLayerSupported = false;
    if (EnableValidationLayers)
    {
        requiredLayers.assign(ValidationLayers.begin(), ValidationLayers.end());
        // Check if the required layers are supported by the Vulkan implementation.
        auto layerProperties = Context.enumerateInstanceLayerProperties();
        auto unsupportedLayerIt = std::ranges::find_if(requiredLayers,
                                                       [&layerProperties](auto const& requiredLayer)
                                                       {
                                                           return std::ranges::none_of(layerProperties,
                                                               [requiredLayer](auto const& layerProperty)
                                                               {
                                                                   return strcmp(layerProperty.layerName, requiredLayer) ==
                                                                       0;
                                                               });
                                                       });
        isLayerSupported = unsupportedLayerIt == requiredLayers.end();
        if (!isLayerSupported)
        {
            Logs::Warning("Required debug layer not supported: %s", *unsupportedLayerIt);
        }
    }

    // Get the required instance extensions from SDL.
    uint32_t sdlExtensionCount = 0;
    auto sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);

    // Check if the required SDL extensions are supported by the Vulkan implementation.
    auto extensionProperties = Context.enumerateInstanceExtensionProperties();
    for (uint32_t i = 0; i < sdlExtensionCount; ++i)
    {
        if (std::ranges::none_of(extensionProperties,
                                 [sdlExtension = sdlExtensions[i]](auto const& extensionProperty)
                                 {
                                     return strcmp(extensionProperty.extensionName, sdlExtension) == 0;
                                 }))
        {
            Logs::Log("Required SDL extension not supported: %s", sdlExtensions[i]);
            SDL_Quit();
            return false;
        }
    }

    vk::InstanceCreateInfo createInfo;
    createInfo.pApplicationInfo = &AppInfo;
    if (isLayerSupported)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
        createInfo.ppEnabledLayerNames = requiredLayers.data();
    }
    createInfo.enabledExtensionCount = sdlExtensionCount;
    createInfo.ppEnabledExtensionNames = sdlExtensions;

    Instance = vk::raii::Instance(Context, createInfo);

    if (isLayerSupported)
        SetupDebugMessenger();

    return true;
}

void Pipeline::SetupDebugMessenger()
{
    if (!EnableValidationLayers)
        return;

    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);

    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{};
    debugUtilsMessengerCreateInfoEXT.messageSeverity = severityFlags;
    debugUtilsMessengerCreateInfoEXT.messageType = messageTypeFlags;
    debugUtilsMessengerCreateInfoEXT.pfnUserCallback = &Pipeline::DebugCallback;

    DebugMessenger = Instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
}

bool Pipeline::PickPhysicalDevice()
{
    std::vector<vk::raii::PhysicalDevice> devices = Instance.enumeratePhysicalDevices();
    if (devices.empty())
    {
        Logs::Error("Failed to find GPUs with Vulkan support!");
        return false;
    }
    // Use an ordered map to automatically sort candidates by increasing score
    for (const auto& device : devices)
    {
        // Check if the device supports the Vulkan 1.4 API version
        bool supportsVulkan1_4 = device.getProperties().apiVersion >= VK_API_VERSION_1_4;

        // Check if any of the queue families support graphics operations
        auto queueFamilies = device.getQueueFamilyProperties();
        bool supportsGraphics = std::ranges::any_of(queueFamilies, [](auto const &qfp) { return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics); });

        // Check if all required device extensions are available
        auto availableDeviceExtensions = device.enumerateDeviceExtensionProperties();
        bool supportsAllRequiredExtensions = std::ranges::all_of(RequiredDeviceExtension,
                                                                   [&availableDeviceExtensions](auto const &requiredDeviceExtension) {
                                                                            return std::ranges::any_of(availableDeviceExtensions,
                                                                                                  [requiredDeviceExtension](auto const &availableDeviceExtension)
                                                                                                  {
                                                                                                      return strcmp(availableDeviceExtension.extensionName, requiredDeviceExtension) == 0;
                                                                                                  });
                                        });

        auto features = device.template getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan14Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
        bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceVulkan14Features>().dynamicRenderingLocalRead &&
                                        features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

        if (supportsVulkan1_4 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures)
        {
            PhysicalDevices.push_back(device);
        }
    }

    if (PhysicalDevices.empty())
    {
        Logs::Error("Failed to find a suitable GPUs!");
        return false;
    }

    PhysicalDevice = PhysicalDevices[0];
    Logs::Log("Device selected: %s", PhysicalDevice.getProperties().deviceName.data());

    return true;
}

void Pipeline::CreateLogicalDevice()
{
    // find the index of the first queue family that supports graphics
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = PhysicalDevice.getQueueFamilyProperties();

    // get the first index into queueFamilyProperties which supports graphics
    auto graphicsQueueFamilyProperty = std::ranges::find_if(queueFamilyProperties, [](auto const &qfp)
    {
        return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0);
    });
    assert(graphicsQueueFamilyProperty != queueFamilyProperties.end() && "No graphics queue family found!");

    auto graphicsIndex = static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));

    // query for Vulkan 1.4 features
    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan14Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain;
    featureChain.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState = true;

    // create a Device
    float queuePriority = 0.5f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo;
    deviceQueueCreateInfo.queueFamilyIndex = graphicsIndex;
    deviceQueueCreateInfo.queueCount = 1;
    deviceQueueCreateInfo.pQueuePriorities = &queuePriority;

    vk::DeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>();
    deviceCreateInfo.pEnabledFeatures = nullptr;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(RequiredDeviceExtension.size());
    deviceCreateInfo.ppEnabledExtensionNames = RequiredDeviceExtension.data();

    Device = vk::raii::Device(PhysicalDevice, deviceCreateInfo);
    GraphicsQueue = vk::raii::Queue(Device, graphicsIndex, 0);
}

void Pipeline::CreateSwapChain()
{
    auto surfaceCapabilities = PhysicalDevice.getSurfaceCapabilitiesKHR(Surface);
    SwapChainExtent = ChooseSwapExtent(surfaceCapabilities);
    SwapChainSurfaceFormat = ChooseSwapSurfaceFormat(PhysicalDevice.getSurfaceFormatsKHR(Surface));
    vk::SwapchainCreateInfoKHR SwapChainCreateInfo;
    SwapChainCreateInfo.surface = Surface;
    SwapChainCreateInfo.minImageCount = ChooseSwapMinImageCount(surfaceCapabilities),
    SwapChainCreateInfo.imageFormat = SwapChainSurfaceFormat.format,
    SwapChainCreateInfo.imageColorSpace = SwapChainSurfaceFormat.colorSpace,
    SwapChainCreateInfo.imageExtent = SwapChainExtent;
    SwapChainCreateInfo.imageArrayLayers = 1;
    SwapChainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    SwapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
    SwapChainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    SwapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    SwapChainCreateInfo.presentMode = ChooseSwapPresentMode(PhysicalDevice.getSurfacePresentModesKHR(Surface));
    SwapChainCreateInfo.clipped = true;

    SwapChain = vk::raii::SwapchainKHR(Device, SwapChainCreateInfo);
    SwapChainImages = SwapChain.getImages();
}

vk::Extent2D Pipeline::ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != 0xFFFFFFFF)
    {
        return capabilities.currentExtent;
    }
    int width, height;
    SDL_GetWindowSizeInPixels(Window, &width, &height);

    return {
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

void Pipeline::CreateGraphicsPipeline()
{
    std::string data;
    ReadTextFile("Resources/Shaders/Slang.spv", data);
    vk::raii::ShaderModule shaderModule = CreateShaderModule(std::vector<char>(data.begin(), data.end()));

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = shaderModule;
    vertShaderStageInfo.pName = "vertMain";

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = shaderModule;
    fragShaderStageInfo.pName = "fragMain";

    vk::PipelineShaderStageCreateInfo shaderStages[] = {
        vertShaderStageInfo,
        fragShaderStageInfo
    };
}

vk::raii::ShaderModule Pipeline::CreateShaderModule(const std::vector<char>& code) const
{
    vk::ShaderModuleCreateInfo createInfo;
    createInfo.codeSize = code.size() * sizeof(char);
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    vk::raii::ShaderModule shaderModule {
        Device,
        createInfo
    };

    return shaderModule;
}

uint32_t Pipeline::ChooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities)
{
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount))
    {
        minImageCount = surfaceCapabilities.maxImageCount;
    }
    return minImageCount;
}

vk::SurfaceFormatKHR Pipeline::ChooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats)
{
    assert(!availableFormats.empty());
    const auto formatIt = std::ranges::find_if(
        availableFormats,
        [](const auto &format)
        {
            return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        });
    return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}

vk::PresentModeKHR Pipeline::ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
{
    assert(std::ranges::any_of(
        availablePresentModes,
        [](auto presentMode)
        {
            return presentMode == vk::PresentModeKHR::eFifo;
        }));
    return std::ranges::any_of(
        availablePresentModes,
        [](const vk::PresentModeKHR value)
        {
            return vk::PresentModeKHR::eMailbox == value;
        }) ?
    vk::PresentModeKHR::eMailbox :
    vk::PresentModeKHR::eFifo;
}

vk::Bool32 Pipeline::DebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                   vk::DebugUtilsMessageTypeFlagsEXT type,
                                   const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
{
    if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError || severity ==
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
    {
        std::string debugType;

        if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral)
        {
            debugType += "General ";
        }
        if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
        {
            debugType += "Validation ";
        }
        if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
        {
            debugType += "Performance ";
        }

        if (debugType.empty())
        {
            debugType = "Unknown ";
        }

        Logs::Error("Validation layer: type %s\nmsg: %s", type, pCallbackData->pMessage);
    }

    return vk::False;
}
