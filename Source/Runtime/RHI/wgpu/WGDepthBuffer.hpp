#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/Settings.hpp"
#include <dawn/webgpu_cpp.h>

class WGDepthBuffer : Object
{
public:
    wgpu::Texture DepthTexture = nullptr;
    wgpu::TextureView DepthTextureView = nullptr;
    wgpu::Sampler DepthTextureSampler = nullptr;

    WGDepthBuffer() = default;
    ~WGDepthBuffer() override = default;

    void Init(wgpu::Device device, wgpu::SurfaceConfiguration& surfaceConfig)
    {
        Deinit();

        wgpu::Extent3D extend = {
            .width = surfaceConfig.width,
            .height = surfaceConfig.height,
            .depthOrArrayLayers = 1
        };

        wgpu::TextureDescriptor depthDesc = {
            .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding,
            .dimension = wgpu::TextureDimension::e2D,
            .size = extend,
            .format = wgpu::TextureFormat::Depth32FloatStencil8,
            .mipLevelCount = 1,
            .sampleCount = GUserSettings->bMSAAEnabled ? uint8_t(4) : uint8_t(1)
        };


        DepthTexture = device.CreateTexture(&depthDesc);

        wgpu::TextureViewDescriptor depthViewDesc = {
            .label = WGPUtils::StrToWgpuStringView("DepthTextureView"),
            .format = wgpu::TextureFormat::Depth32FloatStencil8,
            .dimension = wgpu::TextureViewDimension::e2D,
            .baseMipLevel = 0,
            .mipLevelCount = 1,
            .baseArrayLayer = 0,
            .arrayLayerCount = 1,
        };

        DepthTextureView = DepthTexture.CreateView(&depthViewDesc);

        wgpu::SamplerDescriptor depthSamplerDesc = {
            .addressModeU = wgpu::AddressMode::ClampToEdge,
            .addressModeV = wgpu::AddressMode::ClampToEdge,
            .addressModeW = wgpu::AddressMode::ClampToEdge,
            .magFilter = wgpu::FilterMode::Linear,
            .minFilter = wgpu::FilterMode::Linear,
            .mipmapFilter = wgpu::MipmapFilterMode::Nearest,
            .lodMinClamp = 0.f,
            .lodMaxClamp = 100.f,
            .compare = wgpu::CompareFunction::Less,
            .maxAnisotropy = 1,
        };

        DepthTextureSampler = device.CreateSampler(&depthSamplerDesc);
    }

    void Deinit()
    {

        DepthTextureSampler = nullptr;
        DepthTextureView = nullptr;
        DepthTexture = nullptr;
    }
};
