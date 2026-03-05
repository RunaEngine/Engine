#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan.hpp>

namespace VKUtils
{
    uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

    vk::raii::CommandBuffer BeginSingleTimeCommands();

    void CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& bufferMemory);

    void EndSingleTimeCommands(vk::raii::CommandBuffer& commandBuffer);

    void CopyBuffer(vk::raii::Buffer & srcBuffer, vk::raii::Buffer & dstBuffer, vk::DeviceSize size);

    vk::raii::ImageView CreateImageView(vk::raii::Image& image, vk::Format format);
};