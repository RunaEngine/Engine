#pragma once

#include "Engine/Core/Object.hpp"
#include <webgpu/wgpu.h>

class WGDepthBuffer : Object
{
public:
    bool& MSAAEnabled;
    WGPUTexture DepthTexture = nullptr;
    WGPUTextureView DepthTextureView = nullptr;
    WGPUSampler DepthTextureSampler = nullptr;

    WGDepthBuffer(bool& msaaEnabled) : MSAAEnabled(msaaEnabled)
    {
    }
    ~WGDepthBuffer() override = default;

    void Init(WGPUDevice device, WGPUSurfaceConfiguration& surfaceConfig)
    {
        Deinit();

        WGPUExtent3D extend = {
            .width = surfaceConfig.width,
            .height = surfaceConfig.height,
            .depthOrArrayLayers = 1
        };

        WGPUTextureDescriptor depthDesc = {
            .usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding,
            .dimension = WGPUTextureDimension_2D,
            .size = extend,
            .format = WGPUTextureFormat_Depth32FloatStencil8,
            .mipLevelCount = 1,
            .sampleCount = MSAAEnabled ? uint8_t(4) : uint8_t(1)
        };

        DepthTexture = wgpuDeviceCreateTexture(device, &depthDesc);

        WGPUTextureViewDescriptor depthViewDesc = {
            .format = WGPUTextureFormat_Depth32FloatStencil8,
            .dimension = WGPUTextureViewDimension_2D,
            .baseMipLevel = 0,
            .mipLevelCount = 1,
            .baseArrayLayer = 0,
            .arrayLayerCount = 1,
        };
        DepthTextureView = wgpuTextureCreateView(DepthTexture, &depthViewDesc);

        WGPUSamplerDescriptor depthSamplerDesc = {
            .addressModeU = WGPUAddressMode_ClampToEdge,
            .addressModeV = WGPUAddressMode_ClampToEdge,
            .addressModeW = WGPUAddressMode_ClampToEdge,
            .magFilter = WGPUFilterMode_Linear,
            .minFilter = WGPUFilterMode_Linear,
            .mipmapFilter = WGPUMipmapFilterMode_Nearest,
            .lodMinClamp = 0.f,
            .lodMaxClamp = 100.f,
            .compare = WGPUCompareFunction_Less,
            .maxAnisotropy = 1,
        };

        DepthTextureSampler = wgpuDeviceCreateSampler(device, &depthSamplerDesc);
    }

    void Deinit()
    {
        if (DepthTextureSampler) wgpuSamplerRelease(DepthTextureSampler);
        if (DepthTextureView) wgpuTextureViewRelease(DepthTextureView);
        if (DepthTexture) wgpuTextureRelease(DepthTexture);

        DepthTexture = nullptr;
        DepthTextureView = nullptr;
        DepthTextureSampler = nullptr;
    }
};
