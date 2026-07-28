#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/Utils/Logs.hpp"
#include <dawn/webgpu_cpp.h>
#include <stb_image.h>
#include <filesystem>
#include <array>

class WGTexture : Object
{
private:
    wgpu::Device Device;
    wgpu::Queue Queue;
    stbi_uc* Pixels = nullptr;
    int TexWidth = 0, TexHeight = 0, TexChannels = 0;
    uint32_t MipLevels = 0;
public:
    wgpu::Texture Texture = nullptr;
    wgpu::TextureView TextureView = nullptr;
    wgpu::Sampler TextureSampler = nullptr;
    wgpu::BindGroupLayout TextureBindGroupLayout = nullptr;
    wgpu::BindGroup TextureBindGroup = nullptr;

    WGTexture(wgpu::Device device, wgpu::Queue queue) : Device(device), Queue(queue)
    {
    }

    ~WGTexture() override
    {
        Deinit();
    }

    bool Init(const std::filesystem::path& filepath)
    {
        // Force 4 channels (RGBA) using STBI_rgb_alpha
        Pixels = stbi_load(filepath.string().c_str(), &TexWidth, &TexHeight, &TexChannels, STBI_rgb_alpha);
        //MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        if (!Pixels)
        {
            std::string errorMessage = "Failed to load texture image from " + filepath.string();
            Logs::Error(errorMessage.c_str());
            return false;
        }

        return LoadTexture();
    }

    bool RewriteTexture()
    {
        return LoadTexture();
    }

    void ReleaseFromGPU()
    {
        Texture = nullptr;
        TextureView = nullptr;
        TextureSampler = nullptr;
        TextureBindGroupLayout = nullptr;
        TextureBindGroup = nullptr;
    }

    void Deinit()
    {
        //stbi_image_free(Pixels);
        //Pixels = nullptr;
        TexWidth = 0; TexHeight = 0; TexChannels = 0;
        TextureBindGroupLayout = nullptr;
        TextureBindGroup = nullptr;
        TextureSampler = nullptr;
        TextureView = nullptr;
        Texture = nullptr;
    }

    bool IsLoaded() const
    {
        return Texture != nullptr;
    }

    bool IsValid() const
    {
        return Texture != nullptr;
    }

    uint32_t GetMipLevelCount() const
    {
        return MipLevels;
    }

    wgpu::Extent3D GetExtent() const
    {
        return {
            .width = static_cast<uint32_t>(TexWidth),
            .height = static_cast<uint32_t>(TexHeight),
            .depthOrArrayLayers = 1
        };
    }

private:
    bool LoadTexture()
    {
        if (!Pixels) return false;

        MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(TexWidth, TexHeight)))) + 1;

        size_t size = TexWidth * TexHeight * 4;

         wgpu::Extent3D extend = {
            .width = static_cast<uint32_t>(TexWidth),
            .height = static_cast<uint32_t>(TexHeight),
            .depthOrArrayLayers = 1
        };

        wgpu::TextureDescriptor textureDesc = {
            .usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc,
            .dimension = wgpu::TextureDimension::e2D,
            .size = extend,
            .format = wgpu::TextureFormat::RGBA8Unorm,
            .mipLevelCount = MipLevels,
            .sampleCount = 1
        };
        Texture = Device.CreateTexture(&textureDesc);

        wgpu::TexelCopyTextureInfo textureInfo = {
            .texture = Texture,
            .mipLevel = 0,
            .origin = {0, 0, 0},
            .aspect = wgpu::TextureAspect::All,
        };
        wgpu::TexelCopyBufferLayout bufferLayout = {
            .offset = 0,
            .bytesPerRow = extend.width * 4,
            .rowsPerImage = extend.height
        };
        Queue.WriteTexture(&textureInfo, Pixels, size, &bufferLayout, &extend);


        wgpu::TextureViewDescriptor viewDesc = {
            .format = wgpu::TextureFormat::RGBA8Unorm,
            .dimension = wgpu::TextureViewDimension::e2D,
            .baseMipLevel = 0,
            .mipLevelCount = MipLevels,
            .baseArrayLayer = 0,
            .arrayLayerCount = 1,
        };
        TextureView = Texture.CreateView(&viewDesc);

        wgpu::SamplerDescriptor samplerDesc = {
            .addressModeU = wgpu::AddressMode::Repeat,
            .addressModeV = wgpu::AddressMode::Repeat,
            .addressModeW = wgpu::AddressMode::Repeat,
            .magFilter = wgpu::FilterMode::Linear,
            .minFilter = wgpu::FilterMode::Linear,
            .mipmapFilter = wgpu::MipmapFilterMode::Linear,
            .lodMinClamp = 0.0f,
            .lodMaxClamp = static_cast<float>(MipLevels - 1),
            .maxAnisotropy = 16
        };
        TextureSampler = Device.CreateSampler(&samplerDesc);;

        std::array<wgpu::BindGroupLayoutEntry, 2> bindGroupLayoutEntries;
        wgpu::BindGroupLayoutEntry bindGroupTextureEntry = {
            .binding = 0,
            .visibility = wgpu::ShaderStage::Fragment,
            .texture = {
                .sampleType = wgpu::TextureSampleType::Float,
                .viewDimension = wgpu::TextureViewDimension::e2D,
                .multisampled = false
            },
        };
        bindGroupLayoutEntries[0] = bindGroupTextureEntry;
        wgpu::BindGroupLayoutEntry bindGroupSamplerEntry = {
            .binding = 1,
            .visibility = wgpu::ShaderStage::Fragment,
            .sampler = {
                .type = wgpu::SamplerBindingType::Filtering,
            }
        };
        bindGroupLayoutEntries[1] = bindGroupSamplerEntry;
        wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc = {
            .entryCount = static_cast<uint32_t>(bindGroupLayoutEntries.size()),
            .entries = bindGroupLayoutEntries.data(),
        };
        TextureBindGroupLayout = Device.CreateBindGroupLayout(&bindGroupLayoutDesc);

        std::array<wgpu::BindGroupEntry, 2> bindGroupEntries;
        wgpu::BindGroupEntry bindGroupTextureView = {
            .binding = 0,
            .textureView = TextureView
        };
        bindGroupEntries[0] = bindGroupTextureView;
        wgpu::BindGroupEntry bindGroupSamplerView = {
            .binding = 1,
            .sampler = TextureSampler
        };
        bindGroupEntries[1] = bindGroupSamplerView;
        wgpu::BindGroupDescriptor bindGroupDesc = {
            .layout = TextureBindGroupLayout,
            .entryCount = static_cast<uint32_t>(bindGroupEntries.size()),
            .entries = bindGroupEntries.data()
        };
        TextureBindGroup = Device.CreateBindGroup(&bindGroupDesc);

        stbi_image_free(Pixels);

        return true;
    }
};
