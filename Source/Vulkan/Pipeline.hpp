#pragma once

#include "Config.hpp"
#include "Settings.hpp"
#include "Tick.hpp"
#include "Input.hpp"
#include "Engine/Core/Object.hpp"
#include "Vulkan/Depth.hpp"
#include "Vulkan/Utils.hpp"
#include "Utils/System.hpp"
#include "Utils/Logs.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_raii.hpp>
#include <map>
#include <algorithm>
#include <functional>

extern GameUserSettings* GUserSettings;
extern Tick* GTick;
extern Input* GInput;

#ifdef NDEBUG
constexpr bool EnableValidationLayers = false;
#else
constexpr bool EnableValidationLayers = true;
#endif

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<char const*> ValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> RequiredDeviceExtension = {
    vk::KHRSwapchainExtensionName
};

class Pipeline : public Object
{
public:
    std::function<void(uint32_t)> OnSwap;
    std::function<void(uint32_t)> OnRender;

    // SDL
    SDL_Window* Window = nullptr;
    vk::SurfaceKHR Surface = nullptr;

    // Vulkan
    vk::ApplicationInfo AppInfo;
    UniquePtr<vk::raii::Context> Context;
    vk::raii::Instance Instance = nullptr;
    std::vector<vk::ExtensionProperties> Extensions;
    vk::raii::DebugUtilsMessengerEXT DebugMessenger = nullptr;
    std::vector<vk::raii::PhysicalDevice> PhysicalDevices;
    vk::raii::PhysicalDevice PhysicalDevice = nullptr;
    vk::raii::Device Device = nullptr;
    uint32_t QueueIndex = ~0;
    vk::raii::Queue Queue = nullptr;
    vk::raii::SwapchainKHR SwapChain = nullptr;
    std::vector<vk::Image> SwapChainImages;
    vk::SurfaceFormatKHR SwapChainSurfaceFormat;
    vk::Extent2D SwapChainExtent;
    std::vector<vk::raii::ImageView> SwapChainImageViews;

    vk::raii::CommandPool CommandPool = nullptr;
    std::vector<vk::raii::CommandBuffer> CommandBuffers;

    std::vector<vk::raii::Semaphore> PresentCompleteSemaphores;
    std::vector<vk::raii::Semaphore> RenderFinishedSemaphores;
    std::vector<vk::raii::Fence> InFlightFences;
    uint32_t FrameIndex = 0;

    vk::raii::Fence DrawFence = nullptr;
    bool FramebufferResized = false;

    UniquePtr<VKDepth> DepthBuffer = MakeUnique<VKDepth>();

    Pipeline() = default;

    ~Pipeline() override
    {
        Deinit();
    }

    bool Init()
    {
        if (!CreateInstance())
        {
            Deinit();
            return false;
        }
        if (!PickPhysicalDevice())
        {
            Deinit();
            return false;
        }
        if (!CreateWindow())
        {
            Deinit();
            return false;
        }
        if (!CreateLogicalDevice())
        {
            Deinit();
            return false;
        }
        CreateSwapChain();
        CreateImageViews();
        CreateCommandPool();
        CreateDepthResources();
        CreateCommandBuffers();
        CreateSyncObjects();
        return true;
    }

    void Deinit()
    {
        DebugMessenger.release();
        Extensions.clear();
        Extensions.shrink_to_fit();
        Instance.release();
        PhysicalDevices.clear();
        PhysicalDevices.shrink_to_fit();
        PhysicalDevice.release();

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

        SDL_Vulkan_UnloadLibrary();

        if (SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)
        {
            SDL_Quit();
        }
    }

    void Pool()
    {
        DrawFrame();
    }

private:
    // SDL Funcitons
    bool CreateWindow()
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

        SDL_AddEventWatch(ResizeEventWatcher, this);

        return true;
    }

    static bool SDLCALL ResizeEventWatcher(void* userdata, SDL_Event* event)
    {
        Pipeline* self = (Pipeline*)userdata;
        if (event->type == SDL_EVENT_WINDOW_RESIZED)
        {
            //int newWidth = event->window.data1;
            //int newHeight = event->window.data2;
            self->FramebufferResized = true;
        }
        return true;
    }

    // Vulkan Functions
    void DrawFrame()
    {
        uint64_t frame_time = 0;
        if (GUserSettings->UseVsync)
        {
            SDL_DisplayID displayID = SDL_GetDisplayForWindow(Window);
            if (displayID == 0) {
                Logs::SdlError();
                GUserSettings->UseVsync = false;
                DrawFrame();
                return;
            }

            const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displayID);
            uint16_t fps = int(std::round(mode->refresh_rate));
            frame_time = 1000000000 / fps;
        }
        else
        {
            if (GUserSettings->GetFramerateLimit() > 0)
                frame_time = 1000000000 / GUserSettings->GetFramerateLimit();
        }

        auto fenceResult = Device.waitForFences(*InFlightFences[FrameIndex], vk::True, UINT64_MAX);
        if (fenceResult != vk::Result::eSuccess)
        {
            Logs::RuntimeError("%d\nFailed to wait for fence", fenceResult);
            return;
        }

        auto [result, imageIndex] = SwapChain.acquireNextImage(
            UINT64_MAX, *PresentCompleteSemaphores[FrameIndex], nullptr);
        // Due to VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS being defined, eErrorOutOfDateKHR can be checked as a result
        // here and does not need to be caught by an exception.
        if (result == vk::Result::eErrorOutOfDateKHR)
        {
            RecreateSwapChain();
            return;
        }
        // On other success codes than eSuccess and eSuboptimalKHR we just throw an exception.
        // On any error code, aquireNextImage already threw an exception.
        else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
        {
            assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
            Logs::RuntimeError("%d\nFailed to acquire swap chain image", result);
            return;
        }

        if (OnSwap) OnSwap(FrameIndex);

        // Only reset the fence if we are submitting work
        Device.resetFences(*InFlightFences[FrameIndex]);

        CommandBuffers[FrameIndex].reset();
        RecordCommandBuffer(imageIndex);

        vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        vk::SubmitInfo submitInfo;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &*PresentCompleteSemaphores[FrameIndex];
        submitInfo.pWaitDstStageMask = &waitDestinationStageMask;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &*CommandBuffers[FrameIndex];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &*RenderFinishedSemaphores[imageIndex];

        Queue.submit(submitInfo, *InFlightFences[FrameIndex]);

        vk::PresentInfoKHR presentInfoKHR;
        presentInfoKHR.waitSemaphoreCount = 1;
        presentInfoKHR.pWaitSemaphores = &*RenderFinishedSemaphores[imageIndex];
        presentInfoKHR.swapchainCount = 1;
        presentInfoKHR.pSwapchains = &*SwapChain;
        presentInfoKHR.pImageIndices = &imageIndex;

        result = Queue.presentKHR(presentInfoKHR);
        // Due to VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS being defined, eErrorOutOfDateKHR can be checked as a result
        // here and does not need to be caught by an exception.
        if ((result == vk::Result::eSuboptimalKHR) || (result == vk::Result::eErrorOutOfDateKHR) || FramebufferResized)
        {
            FramebufferResized = false;
            RecreateSwapChain();
        }
        else
        {
            // There are no other success codes than eSuccess; on any error code, presentKHR already threw an exception.
            assert(result == vk::Result::eSuccess);
        }

        FrameIndex = (FrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;

        if (frame_time > 0 && frame_time > GTick->ElapsedNS())
        {
            SDL_DelayPrecise(frame_time - GTick->ElapsedNS());
        }
    }

    void RecreateSwapChain()
    {
        int width = 0, height = 0;
        while (width == 0 || height == 0)
        {
            if (!SDL_GetWindowSizeInPixels(Window, &width, &height))
            {
                Logs::SdlError();
            }
            SDL_WaitEvent(NULL);
        }

        Device.waitIdle();

        CleanupSwapChain();
        CreateSwapChain();
        CreateImageViews();
        CreateDepthResources();
    }

    bool CreateInstance()
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

        //PFN_vkGetInstanceProcAddr instanceProcAdd = (PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr();
        auto ctx = new vk::raii::Context();
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
                            return strcmp(
                                layerProperty.layerName,
                                requiredLayer) ==
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

    void SetupDebugMessenger()
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

    bool PickPhysicalDevice()
    {
        std::vector<vk::raii::PhysicalDevice> devices = Instance.enumeratePhysicalDevices();
        if (devices.empty())
        {
            Logs::Error("Failed to find GPUs with Vulkan support");
            return false;
        }
        // Use an ordered map to automatically sort candidates by increasing score
        for (const auto& device : devices)
        {
            // Check if the device supports the Vulkan 1.4 API version
            bool supportsVulkan = device.getProperties().apiVersion >= VK_API_VERSION_1_4;

            // Check if any of the queue families support graphics operations
            auto queueFamilies = device.getQueueFamilyProperties();
            bool supportsGraphics = std::ranges::any_of(queueFamilies, [](auto const& qfp)
                {
                    return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics);
                });

            // Check if all required device extensions are available
            auto availableDeviceExtensions = device.enumerateDeviceExtensionProperties();
            bool supportsAllRequiredExtensions = std::ranges::all_of(RequiredDeviceExtension,
                [&availableDeviceExtensions](
                    auto const& requiredDeviceExtension)
                {
                    return std::ranges::any_of(
                        availableDeviceExtensions,
                        [requiredDeviceExtension](
                            auto const& availableDeviceExtension)
                        {
                            return strcmp(
                                availableDeviceExtension.
                                extensionName,
                                requiredDeviceExtension) == 0;
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
            Logs::Error("Failed to find a suitable GPU");
            return false;
        }

        PhysicalDevice = PhysicalDevices[0];
        Logs::Warning("Device selected: %s", PhysicalDevice.getProperties().deviceName.data());

        return true;
    }

    bool CreateLogicalDevice()
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
            Logs::Error("Could not find a queue for graphics and present -> terminating");
            return 0;
        }

        // query for Vulkan 1.4 features
        vk::StructureChain<
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceVulkan11Features,
            vk::PhysicalDeviceVulkan13Features,
            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
        > featureChain;

        featureChain.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy = true;
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

        return true;
    }

    void CreateSwapChain()
    {
        auto surfaceCapabilities = PhysicalDevice.getSurfaceCapabilitiesKHR(Surface);
        SwapChainExtent = ChooseSwapExtent(surfaceCapabilities);
        SwapChainSurfaceFormat = ChooseSwapSurfaceFormat(PhysicalDevice.getSurfaceFormatsKHR(Surface));
        vk::SwapchainCreateInfoKHR SwapChainCreateInfo;
        SwapChainCreateInfo.surface = Surface;
        SwapChainCreateInfo.minImageCount = ChooseSwapMinImageCount(surfaceCapabilities);
        SwapChainCreateInfo.imageFormat = SwapChainSurfaceFormat.format;
        SwapChainCreateInfo.imageColorSpace = SwapChainSurfaceFormat.colorSpace;
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

    void CleanupSwapChain()
    {
        SwapChainImageViews.clear();
        SwapChain = nullptr;
    }

    void CreateImageViews()
    {
        assert(SwapChainImageViews.empty());

        SwapChainImageViews.reserve(SwapChainImages.size());
        for (auto& image : SwapChainImages)
        {
            SwapChainImageViews.emplace_back(VKUtils::CreateImageView(image, SwapChainSurfaceFormat.format, vk::ImageAspectFlagBits::eColor, 1));
        }
    }

    vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
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

    static uint32_t ChooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities)
    {
        auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
        if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount))
        {
            minImageCount = surfaceCapabilities.maxImageCount;
        }
        return minImageCount;
    }

    static vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats)
    {
        assert(!availableFormats.empty());
        const auto formatIt = std::ranges::find_if(
            availableFormats,
            [](const auto& format)
            {
                return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace ==
                    vk::ColorSpaceKHR::eSrgbNonlinear;
            });
        return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
    }

    static vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
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
            })
            ? GUserSettings->UseVsync ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eMailbox
            : vk::PresentModeKHR::eFifo;
    }

    void TransitionImageLayout(
        vk::Image image,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::AccessFlags2 srcAccessMask,
        vk::AccessFlags2 dstAccessMask,
        vk::PipelineStageFlags2 srcStageMask,
        vk::PipelineStageFlags2 dstStageMask,
        vk::ImageAspectFlags imageAspectFlags
    )
    {
        vk::ImageSubresourceRange sourceRange;
        sourceRange.aspectMask = imageAspectFlags;
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
        barrier.image = image;
        barrier.subresourceRange = sourceRange;

        vk::DependencyInfo dependencyInfo;
        dependencyInfo.dependencyFlags = {};
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &barrier;

        CommandBuffers[FrameIndex].pipelineBarrier2(dependencyInfo);
    }

    void RecordCommandBuffer(uint32_t imageIndex)
    {
        auto& commandBuffer = CommandBuffers[FrameIndex];
        commandBuffer.begin({});

        // Transition the image layout for rendering
        TransitionImageLayout(
            SwapChainImages[imageIndex],
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            {},
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::ImageAspectFlagBits::eColor
        );

        // Transition depth image to depth attachment optimal layout
        TransitionImageLayout(
            *DepthBuffer->DepthImage,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthAttachmentOptimal,
            vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
            vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
            vk::ImageAspectFlagBits::eDepth);


        // Set up the color attachment
        vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
        vk::RenderingAttachmentInfo attachmentInfo;
        attachmentInfo.imageView = SwapChainImageViews[imageIndex];
        attachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
        attachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
        attachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
        attachmentInfo.clearValue = clearColor;

        // Set up the deph attachment
        vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);
        vk::RenderingAttachmentInfo depthAttachmentInfo;
        depthAttachmentInfo.imageView = DepthBuffer->DepthImageView;
        depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
        depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
        depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
        depthAttachmentInfo.clearValue = clearDepth;

        // Set up the rendering info
        vk::Rect2D renderArea;
        renderArea.offset.x = 0;
        renderArea.offset.y = 0;
        renderArea.extent = SwapChainExtent;

        vk::RenderingInfo renderingInfo;
        renderingInfo.renderArea = renderArea;
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &attachmentInfo;
        renderingInfo.pDepthAttachment = &depthAttachmentInfo;

        // Begin rendering
        commandBuffer.beginRendering(renderingInfo);

        // Rendering commands will go here
        if (OnRender) OnRender(FrameIndex);

        // End rendering
        commandBuffer.endRendering();

        // Transition the image layout for presentation
        TransitionImageLayout(
            SwapChainImages[imageIndex],
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            {},
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eBottomOfPipe,
            vk::ImageAspectFlagBits::eColor
        );

        commandBuffer.end();
    }

    void CreateCommandPool()
    {
        vk::CommandPoolCreateInfo poolInfo;
        poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        poolInfo.queueFamilyIndex = QueueIndex;
        CommandPool = vk::raii::CommandPool(Device, poolInfo);
    }

    void CreateDepthResources()
    {
        DepthBuffer->Init(SwapChainExtent, Device);
    }

    void CreateCommandBuffers()
    {
        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.commandPool = *CommandPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

        CommandBuffers = vk::raii::CommandBuffers(Device, allocInfo);
    }

    void CreateSyncObjects()
    {
        assert(PresentCompleteSemaphores.empty() && RenderFinishedSemaphores.empty() && InFlightFences.empty());

        for (size_t i = 0; i < SwapChainImages.size(); i++)
        {
            RenderFinishedSemaphores.emplace_back(Device, vk::SemaphoreCreateInfo());
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            PresentCompleteSemaphores.emplace_back(Device, vk::SemaphoreCreateInfo());
            vk::FenceCreateInfo createFanceInfo;
            createFanceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
            InFlightFences.emplace_back(Device, createFanceInfo);
        }
    }

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void*)
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
};
