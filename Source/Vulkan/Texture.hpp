#pragma once

#include "Engine/Core/Object.hpp"
#include "Utils/Logs.hpp"
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan.hpp>
#include <stb_image.h>
#include <iostream>

class VKTexture : public Object
{
public:
    uint32_t MipLevels = 0;
    vk::raii::Image TextureImage = nullptr;
    vk::raii::DeviceMemory TextureImageMemory = nullptr;
    vk::raii::ImageView TextureImageView = nullptr;
    vk::raii::Sampler TextureSampler = nullptr;

    VKTexture() = default;
    ~VKTexture() override
    {
        Deinit();
    }

    bool Init(const std::filesystem::path& filepath)
    {
        /*
        SDL_Surface* surf = nullptr;
        // Convert to 8 bits channel space if necessary
        {
            SDL_Surface* rawSurf = IMG_Load(filepath.string().c_str());
            if (!rawSurf)
            {
                Logs::SdlError();
                return false;
            }

            const SDL_PixelFormatDetails* details = SDL_GetPixelFormatDetails(rawSurf->format);

            if (rawSurf->format != SDL_PIXELFORMAT_RGBA32) {
                surf = SDL_ConvertSurfaceAndColorspace(rawSurf, SDL_PIXELFORMAT_RGBA32, nullptr, SDL_COLORSPACE_RGB_DEFAULT, 0);
                if (!surf)
                {
                    Logs::SdlError();
                    return false;
                }
                SDL_DestroySurface(rawSurf);
            } else {
                surf = rawSurf;
            }
        }

        const SDL_PixelFormatDetails* details = SDL_GetPixelFormatDetails(surf->format);
        if (!details)
        {
            Logs::SdlError();
            SDL_DestroySurface(surf);
            return false;
        }
        */

        int texWidth, texHeight, texChannels;

        // Force 4 channels (RGBA) using STBI_rgb_alpha
        stbi_uc* pixels = stbi_load(filepath.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        if (!pixels) {
            std::string errorMessage = "Failed to load texture image from " + filepath.string();
            Logs::Error(errorMessage.c_str());
            return false;
        }

        // Calculate total byte size (4 bytes per pixel)
        VkDeviceSize imageSize = texWidth * texHeight * texChannels;

        vk::raii::Buffer stagingBuffer({});
        vk::raii::DeviceMemory stagingBufferMemory({});

        VKUtils::CreateBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

        void* data = stagingBufferMemory.mapMemory(0, imageSize);
        memcpy(data, pixels, imageSize);
        stagingBufferMemory.unmapMemory();

        std::tie(TextureImage, TextureImageMemory) = VKUtils::CreateImage(texWidth, texHeight, MipLevels, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal);

        vk::raii::CommandBuffer commandBuffer = VKUtils::BeginSingleTimeCommands();
        TransitionImageLayout(TextureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, MipLevels);
        CopyBufferToImage(stagingBuffer, TextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        //TransitionImageLayout(TextureImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, MipLevels);
        VKUtils::GenerateMipmaps(commandBuffer, TextureImage, vk::Format::eR8G8B8A8Srgb, texWidth, texHeight, MipLevels);
        VKUtils::EndSingleTimeCommands(commandBuffer);

        CreateTextureImageView();
        CreateTextureSampler();

        stbi_image_free(pixels);

        return true;
    }

    void Deinit()
    {
        TextureImageView.release();
        TextureImageMemory.release();
        TextureImage.release();
    }
private:
    void TransitionImageLayout(const vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels) {
        auto commandBuffer = VKUtils::BeginSingleTimeCommands();

        vk::ImageMemoryBarrier barrier;
        barrier.oldLayout = oldLayout; 
        barrier.newLayout = newLayout;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor; 
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.layerCount = 1;


        vk::PipelineStageFlags sourceStage;
        vk::PipelineStageFlags destinationStage;

        if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
        {
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

            sourceStage      = vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage = vk::PipelineStageFlagBits::eTransfer;
        }
        else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
        {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            sourceStage      = vk::PipelineStageFlagBits::eTransfer;
            destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
        }
        else
        {
            Logs::RuntimeError("Unsupported layout transition");
        }
        commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, nullptr, barrier);
        VKUtils::EndSingleTimeCommands(commandBuffer);
    }

    void CopyBufferToImage(const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height) {
        vk::raii::CommandBuffer commandBuffer = VKUtils::BeginSingleTimeCommands();
        vk::BufferImageCopy region;
        region.bufferOffset = 0, region.bufferRowLength = 0, region.bufferImageHeight = 0, region.imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        region.imageOffset.x = 0, region.imageOffset.y = 0, region.imageOffset.z = 0, region.imageExtent.width = width, region.imageExtent.height = height, region.imageExtent.depth = 1;
		commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, {region});
        VKUtils::EndSingleTimeCommands(commandBuffer);
    }

    void CreateTextureImageView()
    {
        TextureImageView = VKUtils::CreateImageView(TextureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, MipLevels);
    }

    void CreateTextureSampler()
    {
        auto& device = GPipeline->Device;
        auto& physicalDevice = GPipeline->PhysicalDevice;

        vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
        vk::SamplerCreateInfo samplerInfo;
        samplerInfo.magFilter = vk::Filter::eLinear;
        samplerInfo.minFilter = vk::Filter::eLinear;
        samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
        samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
        samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
        samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.anisotropyEnable = vk::True;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.compareEnable = vk::False;
        samplerInfo.compareOp = vk::CompareOp::eAlways;
        samplerInfo.minLod = 0;
        samplerInfo.maxLod = vk::LodClampNone;

        TextureSampler = vk::raii::Sampler(device, samplerInfo);
    }
};
