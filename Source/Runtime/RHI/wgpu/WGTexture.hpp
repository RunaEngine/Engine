#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/Utils/Logs.hpp"
#include "Runtime/Settings.hpp"
#include <dawn/webgpu_cpp.h>
#include <stb_image.h>
#include <filesystem>
#include <array>

class WGTexture : Object
{
private:
    wgpu::Device Device;
    wgpu::Queue Queue;
    int TexWidth = 0, TexHeight = 0, TexChannels = 0;
    uint32_t MipLevels = 0;

    wgpu::AddressMode AddressMode = wgpu::AddressMode::Undefined;
    wgpu::FilterMode FilterMode = wgpu::FilterMode::Undefined;
    bool GenerateMipmaps = false;
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

    bool Init(const std::filesystem::path& filepath, wgpu::AddressMode addressMode = wgpu::AddressMode::Repeat, wgpu::FilterMode filterMode = wgpu::FilterMode::Linear, bool generateMipmaps = false)
    {
        // Force 4 channels (RGBA) using STBI_rgb_alpha
        stbi_uc* pixels = stbi_load(filepath.string().c_str(), &TexWidth, &TexHeight, &TexChannels, STBI_rgb_alpha);

        if (!pixels)
        {
            std::string errorMessage = "Failed to load texture image from " + filepath.string();
            Logs::Error(errorMessage.c_str());
            return false;
        }

        AddressMode = addressMode;
        FilterMode = filterMode;
        GenerateMipmaps = generateMipmaps;

        if (generateMipmaps)
            MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(TexWidth, TexHeight)))) + 1;

        size_t size = TexWidth * TexHeight * 4;

         wgpu::Extent3D extend = {
            .width = static_cast<uint32_t>(TexWidth),
            .height = static_cast<uint32_t>(TexHeight),
            .depthOrArrayLayers = 1
        };

        wgpu::TextureDescriptor textureDesc = {
            .label = WGPUtils::StrToWgpuStringView(filepath.filename().string()),
            .usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc,
            .dimension = wgpu::TextureDimension::e2D,
            .size = extend,
            .format = wgpu::TextureFormat::RGBA8Unorm,
            .sampleCount = 1
        };
        if (generateMipmaps) textureDesc.mipLevelCount = MipLevels;
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
        Queue.WriteTexture(&textureInfo, pixels, size, &bufferLayout, &extend);


        wgpu::TextureViewDescriptor viewDesc = {
            .label = WGPUtils::StrToWgpuStringView("TextureView"),
            .format = wgpu::TextureFormat::RGBA8Unorm,
            .dimension = wgpu::TextureViewDimension::e2D,
            .baseMipLevel = 0,
            .baseArrayLayer = 0,
            .arrayLayerCount = 1,
        };
        if (generateMipmaps) viewDesc.mipLevelCount = MipLevels;
        TextureView = Texture.CreateView(&viewDesc);

        wgpu::SamplerDescriptor samplerDesc = {
            .label = WGPUtils::StrToWgpuStringView("TextureSampler"),
            .addressModeU = addressMode,
            .addressModeV = addressMode,
            .addressModeW = addressMode,
            .magFilter = GUserSettings->Anisotropic != eDisabled ? wgpu::FilterMode::Linear :  filterMode,
            .minFilter = filterMode,
            .mipmapFilter = (wgpu::MipmapFilterMode)(GUserSettings->Anisotropic != eDisabled ? wgpu::FilterMode::Linear : filterMode),
            .lodMinClamp = 0.0f,
            .lodMaxClamp = generateMipmaps ? static_cast<float>(MipLevels - 1) : 0.0f,
            .maxAnisotropy = uint8_t(GUserSettings->Anisotropic),
        };
        TextureSampler = Device.CreateSampler(&samplerDesc);

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
            .label = WGPUtils::StrToWgpuStringView((filepath.filename().string() + "_bind_group_layout")),
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
            .label = WGPUtils::StrToWgpuStringView("TextureBindGroup"),
            .layout = TextureBindGroupLayout,
            .entryCount = static_cast<uint32_t>(bindGroupEntries.size()),
            .entries = bindGroupEntries.data()
        };
        TextureBindGroup = Device.CreateBindGroup(&bindGroupDesc);

        stbi_image_free(pixels);

        return true;
    }

    void Deinit()
    {
        AddressMode = wgpu::AddressMode::Undefined;
        FilterMode = wgpu::FilterMode::Undefined;
        GenerateMipmaps = false;
        TexWidth = 0; TexHeight = 0; TexChannels = 0;
        MipLevels = 0;
        TextureBindGroupLayout = nullptr;
        TextureBindGroup = nullptr;
        TextureSampler = nullptr;
        TextureView = nullptr;
        Texture = nullptr;
    }

    void UpdateSampler()
    {
        wgpu::SamplerDescriptor samplerDesc = {
            .label = WGPUtils::StrToWgpuStringView("TextureSampler"),
            .addressModeU = AddressMode,
            .addressModeV = AddressMode,
            .addressModeW = AddressMode,
            .magFilter = GUserSettings->Anisotropic != eDisabled ? wgpu::FilterMode::Linear :  FilterMode,
            .minFilter = FilterMode,
            .mipmapFilter = (wgpu::MipmapFilterMode)(GUserSettings->Anisotropic != eDisabled ? wgpu::FilterMode::Linear : FilterMode),
            .lodMinClamp = 0.0f,
            .lodMaxClamp = GenerateMipmaps ? static_cast<float>(MipLevels - 1) : 0.0f,
            .maxAnisotropy = uint8_t(GUserSettings->Anisotropic),
        };
        TextureSampler = Device.CreateSampler(&samplerDesc);

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
            .label = WGPUtils::StrToWgpuStringView("TextureBindGroupLayout"),
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
            .label = WGPUtils::StrToWgpuStringView("TextureBindGroup"),
            .layout = TextureBindGroupLayout,
            .entryCount = static_cast<uint32_t>(bindGroupEntries.size()),
            .entries = bindGroupEntries.data()
        };
        TextureBindGroup = Device.CreateBindGroup(&bindGroupDesc);
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
};
