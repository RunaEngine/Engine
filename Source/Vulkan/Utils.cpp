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

    vk::raii::ImageView CreateImageView(vk::raii::Image& image, vk::Format format)
    {
        auto& device = GPipeline->Device;

        vk::ImageViewCreateInfo viewInfo;
        viewInfo.image = image,
            viewInfo.viewType = vk::ImageViewType::e2D,
            viewInfo.format = format,
            viewInfo.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
        return vk::raii::ImageView(device, viewInfo);
    }
}