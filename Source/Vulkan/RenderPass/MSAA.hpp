#pragma once

#include "Engine/Core/Object.hpp"
#include <vulkan/vulkan_raii.hpp>

#include "Vulkan/Utils.hpp"

class VKMSAA : public Object
{
private:
    vk::raii::PhysicalDevice& PhysicalDevice;
    vk::SurfaceFormatKHR& SwapChainSurfaceFormat;
    vk::Extent2D& SwapChainExtent;
public:
    vk::raii::Image ColorImage = nullptr;
    vk::raii::DeviceMemory ColorImageMemory = nullptr;
    vk::raii::ImageView ColorImageView = nullptr;

    vk::SampleCountFlagBits MSAASamples = vk::SampleCountFlagBits::e1;

    VKMSAA() = default;
    VKMSAA(vk::raii::PhysicalDevice& physicalDevice, vk::SurfaceFormatKHR& swapChainSurfaceFormat, vk::Extent2D& swapChainExtent) : PhysicalDevice(physicalDevice), SwapChainSurfaceFormat(swapChainSurfaceFormat), SwapChainExtent(swapChainExtent) {}
    ~VKMSAA() = default;

    void Init()
    {
        MSAASamples = GetMaxUsableSampleCount();
        CreateColorResources();
    }

    void Deinit()
    {
        ColorImageView.release();
        ColorImageMemory.release();
        ColorImage.release();
    }

    vk::SampleCountFlagBits GetMaxUsableSampleCount() {
        vk::PhysicalDeviceProperties physicalDeviceProperties = PhysicalDevice.getProperties();

        vk::SampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
        if (counts & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
        if (counts & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
        if (counts & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
        if (counts & vk::SampleCountFlagBits::e8) { return vk::SampleCountFlagBits::e8; }
        if (counts & vk::SampleCountFlagBits::e4) { return vk::SampleCountFlagBits::e4; }
        if (counts & vk::SampleCountFlagBits::e2) { return vk::SampleCountFlagBits::e2; }

        return vk::SampleCountFlagBits::e1;
    }

private:
    void CreateColorResources() {
        vk::Format colorFormat = SwapChainSurfaceFormat.format;

        VKUtils::CreateImage(SwapChainExtent.width, SwapChainExtent.height, 1, MSAASamples, colorFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,  vk::MemoryPropertyFlagBits::eDeviceLocal, ColorImage, ColorImageMemory);
        ColorImageView = VKUtils::CreateImageView(ColorImage, colorFormat, vk::ImageAspectFlagBits::eColor, 1);
    }
};
