#pragma once

#include "Config.h"
#include "Engine/Core/Object.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_raii.hpp>

#ifdef ENGINE_BUILD_DEBUG
constexpr bool EnableValidationLayers = false;
#else
constexpr bool EnableValidationLayers = false;
#endif

class Pipeline : public Object
{
public:
    Pipeline() = default;
    ~Pipeline() override;

    bool Init();
    void Deinit();

    void Pool();
private:
    // SDL
    SDL_Window* Window = nullptr;
    VkSurfaceKHR Surface = nullptr;

    // Vulkan
    vk::ApplicationInfo AppInfo;
    vk::raii::Context Context;
    vk::raii::Instance Instance = nullptr;
    std::vector<vk::ExtensionProperties> Extensions;
    vk::raii::DebugUtilsMessengerEXT DebugMessenger = nullptr;
    std::vector<vk::raii::PhysicalDevice> PhysicalDevices;
    vk::raii::PhysicalDevice PhysicalDevice = nullptr;
    vk::raii::Device Device = nullptr;
    vk::raii::Queue GraphicsQueue = nullptr;
    vk::raii::SwapchainKHR SwapChain = nullptr;
    std::vector<vk::Image> SwapChainImages;
    vk::SurfaceFormatKHR SwapChainSurfaceFormat;
    vk::Extent2D SwapChainExtent;
    std::vector<vk::raii::ImageView> SwapChainImageViews;

    // SDL Funcitons
    bool CreateWindow();

    // Vulkan Functions
    bool CreateInstance();
    void SetupDebugMessenger();
    bool PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapChain();
    vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities);
    void CreateGraphicsPipeline();
    [[nodiscard]] vk::raii::ShaderModule CreateShaderModule(const std::vector<char> &code) const;
    static uint32_t ChooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const &surfaceCapabilities);
    static vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const &availableFormats);
    static vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes);
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData, void *);
};