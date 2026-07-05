#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan.hpp>

namespace VKUtils
{
    uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

    vk::raii::CommandBuffer BeginSingleTimeCommands();

    void EndSingleTimeCommands(vk::raii::CommandBuffer& commandBuffer);

    void CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& bufferMemory);

    void CopyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size);

    void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Image &image, vk::raii::DeviceMemory &imageMemory);

    vk::raii::ImageView CreateImageView(vk::Image const& image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels);

    vk::Format FindSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);

    vk::Format FindDepthFormat();

    void GenerateMipmaps(vk::raii::CommandBuffer& commandBuffer, vk::raii::Image& image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
};