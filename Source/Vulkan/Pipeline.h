#pragma once
#define VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL 0
#define VK_NO_PROTOTYPES

#include "Config.h"
#include "Engine/Core/Object.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_raii.hpp>
#include <functional>

#ifdef ENGINE_BUILD_DEBUG
constexpr bool EnableValidationLayers = true;
#else
constexpr bool EnableValidationLayers = false;
#endif

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

class Pipeline : public Object
{
public:
    Pipeline() = default;
    ~Pipeline() override;

    bool Init();
    void Deinit();

    void Pool();

    vk::raii::CommandBuffer& GetCommandBuffer();
    vk::raii::Device& GetDevice();
    vk::SurfaceFormatKHR& GetSwapChainSurfaceFormat();
    vk::Extent2D& GetSwapChainExtent();

    std::function<void()> OnRender;
private:
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
    //vk::raii::Queue GraphicsQueue = nullptr;
    vk::raii::SwapchainKHR SwapChain = nullptr;
    std::vector<vk::Image> SwapChainImages;
    vk::SurfaceFormatKHR SwapChainSurfaceFormat;
    vk::Extent2D SwapChainExtent;
    std::vector<vk::raii::ImageView> SwapChainImageViews;

    vk::raii::CommandPool CommandPool = nullptr;
    vk::raii::CommandBuffer CommandBuffer = nullptr;

    vk::raii::Semaphore PresentCompleteSemaphore = nullptr;
    vk::raii::Semaphore RenderFinishedSemaphore = nullptr;
    vk::raii::Fence DrawFence = nullptr;

    // SDL Funcitons
    bool CreateWindow();

    // Vulkan Functions
    void DrawFrame();

    bool CreateInstance();
    void SetupDebugMessenger();
    bool PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapChain();
    void CreateImageViews();
    vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities);
    static uint32_t ChooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const &surfaceCapabilities);
    static vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const &availableFormats);
    static vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes);

    void transition_image_layout(
        uint32_t imageIndex,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::AccessFlags2 srcAccessMask,
        vk::AccessFlags2 dstAccessMask,
        vk::PipelineStageFlags2 srcStageMask,
        vk::PipelineStageFlags2 dstStageMask
    );
    void RecordCommandBuffer(uint32_t imageIndex);
    void CreateCommandPool();
    void CreateCommandBuffer();

    void CreateSyncObjects();

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData, void *);
};