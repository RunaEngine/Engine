#pragma once

#include "Engine/Core/Object.hpp"
#include <vulkan/vulkan_raii.hpp>
#include "Vulkan/Utils.hpp"

class VKDepth : public Object
{
public:
    vk::raii::Image DepthImage = nullptr;
    vk::raii::DeviceMemory DepthImageMemory = nullptr;
    vk::raii::ImageView DepthImageView = nullptr;

    VKDepth() = default;
    ~VKDepth() override
    {
        Deinit();
    }

    void Init(vk::Extent2D& swapChainExtent, vk::raii::Device& device)
    {
        vk::Format depthFormat = VKUtils::FindDepthFormat();
        std::tie(DepthImage, DepthImageMemory) = VKUtils::CreateImage(swapChainExtent.width, swapChainExtent.height, 1, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal);
        DepthImageView = VKUtils::CreateImageView(DepthImage, depthFormat, vk::ImageAspectFlagBits::eDepth, 1);
    }

    void Deinit()
    {
        DepthImageView.release();
        DepthImageMemory.release();
        DepthImage.release();
    }
};