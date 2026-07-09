#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/Vulkan/Utils.hpp"
#include <vulkan/vulkan_raii.hpp>

class VKColor : public Object
{
private:
    vk::raii::PhysicalDevice& PhysicalDevice;
    vk::SurfaceFormatKHR& SwapChainSurfaceFormat;
    vk::Extent2D& SwapChainExtent;
public:
    vk::raii::Image ColorImage = nullptr;
    vk::raii::DeviceMemory ColorImageMemory = nullptr;
    vk::raii::ImageView ColorImageView = nullptr;

    VKColor() = default;
    VKColor(vk::raii::PhysicalDevice& physicalDevice, vk::SurfaceFormatKHR& swapChainSurfaceFormat, vk::Extent2D& swapChainExtent) : PhysicalDevice(physicalDevice), SwapChainSurfaceFormat(swapChainSurfaceFormat), SwapChainExtent(swapChainExtent) {}
    ~VKColor() = default;

    void Init(vk::SampleCountFlagBits numSamples)
    {
        vk::Format colorFormat = SwapChainSurfaceFormat.format;

        VKUtils::CreateImage(
            SwapChainExtent.width,
            SwapChainExtent.height,
            1,
            numSamples,
            colorFormat,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eColorAttachment,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            ColorImage,
            ColorImageMemory
        );

        ColorImageView = VKUtils::CreateImageView(ColorImage, colorFormat, vk::ImageAspectFlagBits::eColor, 1);
    }

    void Deinit()
    {
        ColorImageView.release();
        ColorImageMemory.release();
        ColorImage.release();
    }
};
