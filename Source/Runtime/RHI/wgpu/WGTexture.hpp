#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/Utils/Logs.hpp"
#include <webgpu/wgpu.h>
#include <stb_image.h>
#include <filesystem>
#include <array>

class WGTexture : Object
{
private:
    WGPUDevice Device = nullptr;
    WGPUQueue Queue = nullptr;
public:
    WGPUTexture Texture = nullptr;
    WGPUTextureView TextureView = nullptr;
    WGPUSampler TextureSampler = nullptr;
    WGPUBindGroupLayout TextureBindGroupLayout = nullptr;
    WGPUBindGroup TextureBindGroup = nullptr;

    WGTexture(WGPUDevice device, WGPUQueue queue) : Device(device), Queue(queue) {}
    ~WGTexture() override = default;

    bool Init(const std::filesystem::path& filepath)
    {
        int texWidth, texHeight, texChannels;

        // Force 4 channels (RGBA) using STBI_rgb_alpha
        stbi_uc* pixels = stbi_load(filepath.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        //MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        if (!pixels)
        {
            std::string errorMessage = "Failed to load texture image from " + filepath.string();
            Logs::Error(errorMessage.c_str());
            return false;
        }

        WGPUExtent3D extend = {
            .width = static_cast<uint32_t>(texWidth),
            .height = static_cast<uint32_t>(texHeight),
            .depthOrArrayLayers = 1
        };

        WGPUTextureDescriptor textureDesc = {
            .usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
            .dimension = WGPUTextureDimension_2D,
            .size = extend,
            .format = WGPUTextureFormat_RGBA8Unorm,
            .mipLevelCount = 1,
            .sampleCount = 1
        };
        Texture = wgpuDeviceCreateTexture(Device, &textureDesc);

        WGPUTexelCopyTextureInfo textureInfo = {
            .texture = Texture,
            .mipLevel = 0,
            .origin = { 0, 0, 0 },
            .aspect = WGPUTextureAspect_All,
        };
        WGPUTexelCopyBufferLayout bufferLayout = {
            .offset = 0,
            .bytesPerRow = extend.width * 4,
            .rowsPerImage = extend.height
        };
        wgpuQueueWriteTexture(
            Queue,
            &textureInfo,
            pixels,
            extend.width * extend.height * 4,
            &bufferLayout,
            &extend
        );

        WGPUTextureViewDescriptor viewDesc = {
            .format = WGPUTextureFormat_RGBA8Unorm,
            .dimension = WGPUTextureViewDimension_2D,
            .baseMipLevel = 0,
            .mipLevelCount = 1,
            .baseArrayLayer = 0,
            .arrayLayerCount = 1,
        };
        TextureView = wgpuTextureCreateView(Texture, &viewDesc);

        WGPUSamplerDescriptor samplerDesc = {
            .addressModeU = WGPUAddressMode_ClampToEdge,
            .addressModeV = WGPUAddressMode_ClampToEdge,
            .addressModeW = WGPUAddressMode_ClampToEdge,
            .magFilter = WGPUFilterMode_Linear,
            .minFilter = WGPUFilterMode_Linear,
            .mipmapFilter = WGPUMipmapFilterMode_Nearest,
            .maxAnisotropy = 1
        };
        TextureSampler = wgpuDeviceCreateSampler(Device, &samplerDesc);

        std::array<WGPUBindGroupLayoutEntry, 2> bindGroupLayoutEntries;
        WGPUBindGroupLayoutEntry bindGroupTextureEntry = {
            .binding = 0,
            .visibility = WGPUShaderStage_Fragment,
            .texture = {
                .sampleType = WGPUTextureSampleType_Float,
                .viewDimension = WGPUTextureViewDimension_2D,
                .multisampled = false
            },
        };
        bindGroupLayoutEntries[0] = bindGroupTextureEntry;
        WGPUBindGroupLayoutEntry bindGroupSamplerEntry = {
            .binding = 1,
            .visibility = WGPUShaderStage_Fragment,
            .sampler = {
                .type = WGPUSamplerBindingType_Filtering,
            }
        };
        bindGroupLayoutEntries[1] = bindGroupSamplerEntry;
        WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {
            .entryCount = static_cast<uint32_t>(bindGroupLayoutEntries.size()),
            .entries = bindGroupLayoutEntries.data(),
        };
        TextureBindGroupLayout = wgpuDeviceCreateBindGroupLayout(Device, &bindGroupLayoutDesc);

        std::array<WGPUBindGroupEntry, 2> bindGroupEntries;
        WGPUBindGroupEntry bindGroupTextureView = {
            .binding = 0,
            .textureView = TextureView
        };
        bindGroupEntries[0] = bindGroupTextureView;
        WGPUBindGroupEntry bindGroupSamplerView = {
            .binding = 1,
            .sampler = TextureSampler
        };
        bindGroupEntries[1] = bindGroupSamplerView;
        WGPUBindGroupDescriptor bindGroupDesc = {
            .layout = TextureBindGroupLayout,
            .entryCount = static_cast<uint32_t>(bindGroupEntries.size()),
            .entries = bindGroupEntries.data()
        };
        TextureBindGroup = wgpuDeviceCreateBindGroup(Device, &bindGroupDesc);

        stbi_image_free(pixels);

        return true;
    }

    void Deinit()
    {
        if (Texture) 
        {
            wgpuTextureDestroy(Texture);
            wgpuTextureRelease(Texture);
        }
        if (TextureView) wgpuTextureViewRelease(TextureView);
        if (TextureSampler) wgpuSamplerRelease(TextureSampler);
        if (TextureBindGroupLayout) wgpuBindGroupLayoutRelease(TextureBindGroupLayout);
        if (TextureBindGroup) wgpuBindGroupRelease(TextureBindGroup);

        Texture = nullptr;
        TextureView = nullptr;
        TextureSampler = nullptr;
        TextureBindGroupLayout = nullptr;
        TextureBindGroup = nullptr;
    }
};
