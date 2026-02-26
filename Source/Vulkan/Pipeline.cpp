#include "Vulkan/Pipeline.h"
#include "Utils/System.h"
#include "Utils/Logs.h"
#include <map>
#include <algorithm>

const std::vector<char const*> ValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> RequiredDeviceExtension = {
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
    CreateImageViews();
    CreateCommandPool();
    CreateCommandBuffer();
    CreateSyncObjects();
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
    DrawFrame();
}

vk::raii::CommandBuffer& Pipeline::GetCommandBuffer()
{
    return CommandBuffer;
}

vk::raii::Device& Pipeline::GetDevice()
{
    return Device;
}

vk::SurfaceFormatKHR& Pipeline::GetSwapChainSurfaceFormat()
{
    return SwapChainSurfaceFormat;
}

vk::Extent2D& Pipeline::GetSwapChainExtent()
{
    return SwapChainExtent;
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

    VkSurfaceKHR surfaceHandle = nullptr;
    if (!SDL_Vulkan_CreateSurface(Window, *Instance, NULL, &surfaceHandle))
    {
        Logs::SdlError();
        return false;
    }
    Surface = surfaceHandle;

    return true;
}

void Pipeline::DrawFrame()
{
    // NOTE: for simplicity, wait for the queue to be idle before starting the frame
    Queue.waitIdle();
    // In the next chapter you see how to use multiple frames in flight and fences to sync

    auto [result, imageIndex] = SwapChain.acquireNextImage(UINT64_MAX, *PresentCompleteSemaphore, nullptr);
    RecordCommandBuffer(imageIndex);

    Device.resetFences(*DrawFence);
    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);

    vk::SubmitInfo submitInfo;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &*PresentCompleteSemaphore;
    submitInfo.pWaitDstStageMask = &waitDestinationStageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &*CommandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &*RenderFinishedSemaphore;

    Queue.submit(submitInfo, *DrawFence);
    result = Device.waitForFences(*DrawFence, vk::True, UINT64_MAX);
    if (result != vk::Result::eSuccess)
    {
        Logs::Error("failed to wait for fence!");
        return;
    }

    vk::PresentInfoKHR presentInfoKHR;
    presentInfoKHR.waitSemaphoreCount = 1;
    presentInfoKHR.pWaitSemaphores = &*RenderFinishedSemaphore;
    presentInfoKHR.swapchainCount = 1;
    presentInfoKHR.pSwapchains = &*SwapChain;
    presentInfoKHR.pImageIndices = &imageIndex;

    result = Queue.presentKHR(presentInfoKHR);
    switch (result)
    {
    case vk::Result::eSuccess:
        break;
    case vk::Result::eSuboptimalKHR:
        Logs::Warning("vk::Queue::presentKHR returned vk::Result::eSuboptimalKHR !");
        break;
    default:
        break;        // an unexpected result is returned!
    }
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

    if (!SDL_Vulkan_LoadLibrary(NULL))
    {
        Logs::SdlError();
        SDL_Quit();
        return false;
    }

    PFN_vkGetInstanceProcAddr instanceProcAdd = (PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr();
    auto ctx = new vk::raii::Context(instanceProcAdd);
    Context.reset(ctx);

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
        auto layerProperties = Context->enumerateInstanceLayerProperties();
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

    std::vector<const char*> instanceExtensions;
    for (uint32_t i = 0; i < sdlExtensionCount; ++i)
    {
        instanceExtensions.push_back(sdlExtensions[i]);
    }

    if (EnableValidationLayers)
    {
        instanceExtensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    // Check if the required extensions are supported by the Vulkan implementation.
    auto extensionProperties = Context->enumerateInstanceExtensionProperties();
    for (auto const& extName : instanceExtensions)
    {
        if (std::ranges::none_of(extensionProperties,
            [extName](auto const& extensionProperty)
            {
                return strcmp(extensionProperty.extensionName, extName) == 0;
            }))
        {
            Logs::Log("Required extension not supported: %s", extName);
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
    createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();

    Instance = vk::raii::Instance(*Context, createInfo);

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

    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT;
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
        bool supportsVulkan = device.getProperties().apiVersion >= VK_API_VERSION_1_4;

        // Check if any of the queue families support graphics operations
        auto queueFamilies = device.getQueueFamilyProperties();
        bool supportsGraphics = std::ranges::any_of(queueFamilies, [](auto const& qfp) { return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics); });

        // Check if all required device extensions are available
        auto availableDeviceExtensions = device.enumerateDeviceExtensionProperties();
        bool supportsAllRequiredExtensions = std::ranges::all_of(RequiredDeviceExtension,
            [&availableDeviceExtensions](auto const& requiredDeviceExtension) {
                return std::ranges::any_of(availableDeviceExtensions,
                    [requiredDeviceExtension](auto const& availableDeviceExtension)
                    {
                        return strcmp(availableDeviceExtension.extensionName, requiredDeviceExtension) == 0;
                    });
            });

        auto features = device.template getFeatures2<
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceVulkan11Features,
            vk::PhysicalDeviceVulkan13Features,
            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

        bool supportsRequiredFeatures = features.template get<
            vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
            features.template get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
            features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
            features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

        if (supportsVulkan && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures)
        {
            PhysicalDevices.push_back(device);
        }
    }

    if (PhysicalDevices.empty())
    {
        throw std::runtime_error("Failed to find a suitable GPU!");
        return false;
    }

    PhysicalDevice = PhysicalDevices[0];
    Logs::Warning("Device selected: %s", PhysicalDevice.getProperties().deviceName.data());

    return true;
}

void Pipeline::CreateLogicalDevice()
{
    // find the index of the first queue family that supports graphics
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = PhysicalDevice.getQueueFamilyProperties();

    // get the first index into queueFamilyProperties which supports both graphics and present
    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
    {
        if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
            PhysicalDevice.getSurfaceSupportKHR(qfpIndex, Surface))
        {
            // found a queue family that supports both graphics and present
            QueueIndex = qfpIndex;
            break;
        }
    }
    if (QueueIndex == ~0)
    {
        throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
    }

    // query for Vulkan 1.4 features
    vk::StructureChain<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
    > featureChain;

    featureChain.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters = true;
    featureChain.get<vk::PhysicalDeviceVulkan13Features>().synchronization2 = true;
    featureChain.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering = true;
    featureChain.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState = true;

    // create a Device
    float queuePriority = 0.5f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo;
    deviceQueueCreateInfo.queueFamilyIndex = QueueIndex;
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
    Queue = vk::raii::Queue(Device, QueueIndex, 0);
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

void Pipeline::CreateImageViews()
{
    assert(SwapChainImageViews.empty());

    vk::ImageViewCreateInfo imageViewCreateInfo;
    imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
    imageViewCreateInfo.format = SwapChainSurfaceFormat.format;
    imageViewCreateInfo.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

    for (auto& image : SwapChainImages)
    {
        imageViewCreateInfo.image = image;
        SwapChainImageViews.emplace_back(Device, imageViewCreateInfo);
    }
}

vk::Extent2D Pipeline::ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != 0xFFFFFFFF)
    {
        return capabilities.currentExtent;
    }
    int width = 1024, height = 576;
    SDL_GetWindowSizeInPixels(Window, &width, &height);

    return {
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
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
        [](const auto& format)
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

void Pipeline::transition_image_layout(uint32_t imageIndex, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
    vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask, vk::PipelineStageFlags2 srcStageMask,
    vk::PipelineStageFlags2 dstStageMask)
{
    vk::ImageSubresourceRange sourceRange;
    sourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    sourceRange.baseMipLevel = 0;
    sourceRange.levelCount = 1;
    sourceRange.baseArrayLayer = 0;
    sourceRange.layerCount = 1;

    vk::ImageMemoryBarrier2 barrier;
    barrier.srcStageMask = srcStageMask;
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstStageMask = dstStageMask;
    barrier.dstAccessMask = dstAccessMask;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = SwapChainImages[imageIndex];
    barrier.subresourceRange = sourceRange;

    vk::DependencyInfo dependencyInfo;
    dependencyInfo.dependencyFlags = {};
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &barrier;

    CommandBuffer.pipelineBarrier2(dependencyInfo);
}

void Pipeline::RecordCommandBuffer(uint32_t imageIndex)
{
    CommandBuffer.begin({});

    // Transition the image layout for rendering
    transition_image_layout(
        imageIndex,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput
    );

    // Set up the color attachment
    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    vk::RenderingAttachmentInfo attachmentInfo;
    attachmentInfo.imageView = SwapChainImageViews[imageIndex];
    attachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    attachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
    attachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
    attachmentInfo.clearValue = clearColor;

    // Set up the rendering info
    vk::Rect2D renderArea;
    renderArea.offset.x = 0; renderArea.offset.y = 0;
    renderArea.extent = SwapChainExtent;

    vk::RenderingInfo renderingInfo;
    renderingInfo.renderArea = renderArea;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &attachmentInfo;

    // Begin rendering
    CommandBuffer.beginRendering(renderingInfo);

    // Rendering commands will go here
    if (OnRender) OnRender();

    // End rendering
    CommandBuffer.endRendering();

    // Transition the image layout for presentation
    transition_image_layout(
        imageIndex,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        {},
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eBottomOfPipe
    );

    CommandBuffer.end();
}

void Pipeline::CreateCommandPool()
{
    vk::CommandPoolCreateInfo poolInfo;
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    poolInfo.queueFamilyIndex = QueueIndex;
    CommandPool = vk::raii::CommandPool(Device, poolInfo);
}

void Pipeline::CreateCommandBuffer()
{
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.commandPool = *CommandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = 1;

    CommandBuffer = std::move(vk::raii::CommandBuffers(Device, allocInfo).front());
}

void Pipeline::CreateSyncObjects()
{
    PresentCompleteSemaphore = vk::raii::Semaphore(Device, vk::SemaphoreCreateInfo());
    RenderFinishedSemaphore = vk::raii::Semaphore(Device, vk::SemaphoreCreateInfo());
    vk::FenceCreateInfo fenceInfo;
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
    DrawFence = vk::raii::Fence(Device, fenceInfo);
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

        Logs::Error("Validation layer: %s\nMessage: %s", debugType.c_str(), pCallbackData->pMessage);
    }

    return vk::False;
}
