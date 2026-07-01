#include "Vulkan/Utils.hpp"
#include "Engine/Engine.hpp"
#include "Utils/Logs.hpp"

namespace VKUtils
{
    uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
    {
        auto& physicalDevice = GPipeline->PhysicalDevice;
        vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        Logs::RuntimeError("Failed to find suitable memory type");
        return 0;
    }

    vk::raii::CommandBuffer BeginSingleTimeCommands()
    {
        auto& device = GPipeline->Device;
        auto& commandPool = GPipeline->CommandPool;

        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.commandPool = commandPool,
        allocInfo.level = vk::CommandBufferLevel::ePrimary,
        allocInfo.commandBufferCount = 1;

        vk::raii::CommandBuffer commandBuffer = std::move(device.allocateCommandBuffers(allocInfo).front());

        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        commandBuffer.begin(beginInfo);

        return commandBuffer;
    }

    void CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& bufferMemory)
    {
        auto& device = GPipeline->Device;

        vk::BufferCreateInfo bufferInfo;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        buffer = vk::raii::Buffer(device, bufferInfo);
        vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
        bufferMemory = vk::raii::DeviceMemory(device, allocInfo);
        buffer.bindMemory(*bufferMemory, 0);
    }

    void EndSingleTimeCommands(vk::raii::CommandBuffer& commandBuffer)
    {
        auto& graphicsQueue = GPipeline->Queue;

        commandBuffer.end();

        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = 1, submitInfo.pCommandBuffers = &*commandBuffer;
        graphicsQueue.submit(submitInfo, nullptr);
        graphicsQueue.waitIdle();
    }

    void CopyBuffer(vk::raii::Buffer & srcBuffer, vk::raii::Buffer & dstBuffer, vk::DeviceSize size)
    {
        vk::raii::CommandBuffer commandCopyBuffer = BeginSingleTimeCommands();
        commandCopyBuffer.copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy(0, 0, size));
        EndSingleTimeCommands(commandCopyBuffer);
    }

    std::pair<vk::raii::Image, vk::raii::DeviceMemory> CreateImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties)
    {
        vk::ImageCreateInfo imageInfo;
        imageInfo.imageType   = vk::ImageType::e2D,
        imageInfo.format      = format;
        imageInfo.extent.width  = width,
        imageInfo.extent.height = height,
        imageInfo.extent.depth  = 1,
        imageInfo.mipLevels   = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples     = vk::SampleCountFlagBits::e1;
        imageInfo.tiling      = tiling;
        imageInfo.usage       = usage;
        imageInfo.sharingMode = vk::SharingMode::eExclusive;

        vk::raii::Image image = vk::raii::Image(GPipeline->Device, imageInfo);

        vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo;
        allocInfo.allocationSize  = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
        vk::raii::DeviceMemory imageMemory = vk::raii::DeviceMemory(GPipeline->Device, allocInfo);
        image.bindMemory(imageMemory, 0);

        return {std::move(image), std::move(imageMemory)};
    }

    vk::raii::ImageView CreateImageView(vk::Image const &image, vk::Format format, vk::ImageAspectFlags aspectFlags)
    {
        auto& device = GPipeline->Device;

        vk::ImageViewCreateInfo viewInfo;
        viewInfo.image = image,
            viewInfo.viewType = vk::ImageViewType::e2D,
            viewInfo.format = format,
            viewInfo.subresourceRange = { aspectFlags, 0, 1, 0, 1 };
        return vk::raii::ImageView(device, viewInfo);
    }

    vk::Format FindSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
        vk::FormatFeatureFlags features)
    {
        vk::raii::PhysicalDevice physicalDevice = GPipeline->PhysicalDevice;
        for (const auto format : candidates) {
            vk::FormatProperties props = physicalDevice.getFormatProperties(format);

            if (((tiling == vk::ImageTiling::eLinear) && ((props.linearTilingFeatures & features) == features)) ||
                ((tiling == vk::ImageTiling::eOptimal) && ((props.optimalTilingFeatures & features) == features)))
            {
                return format;
            }
        }

        Logs::RuntimeError("failed to find supported format!");

        return vk::Format::eUndefined;
    }

    vk::Format FindDepthFormat()
    {
        return FindSupportedFormat({vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
                                   vk::ImageTiling::eOptimal,
                                   vk::FormatFeatureFlagBits::eDepthStencilAttachment);
    }
}
