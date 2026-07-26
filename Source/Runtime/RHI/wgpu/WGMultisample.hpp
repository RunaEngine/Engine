#pragma once

#include "Engine/Core/Object.hpp"
#include <webgpu/wgpu.h>

class WGMultisample : Object
{
public:
    bool Enabled = true;
    bool PreviousEnabled = Enabled;
    WGPUTexture Texture = nullptr;
    WGPUTextureView TextureView = nullptr;

    WGMultisample() = default;
    ~WGMultisample() override = default;

    void Init(WGPUDevice device, WGPUSurfaceConfiguration& surfaceConfig)
    {
        Deinit();

        WGPUExtent3D extend = {
            .width = surfaceConfig.width,
            .height = surfaceConfig.height,
            .depthOrArrayLayers = 1
        };

        WGPUTextureDescriptor msaaDesc = {
            .usage = WGPUTextureUsage_RenderAttachment,
            .dimension = WGPUTextureDimension_2D,
            .size = extend,
            .format = surfaceConfig.format,
            .mipLevelCount = 1,
            .sampleCount = 4
        };

        Texture = wgpuDeviceCreateTexture(device, &msaaDesc);

        WGPUTextureViewDescriptor msaaViewDesc = {
            .format = surfaceConfig.format,
            .dimension = WGPUTextureViewDimension_2D,
            .baseMipLevel = 0,
            .mipLevelCount = 1,
            .baseArrayLayer = 0,
            .arrayLayerCount = 1,
        };
        TextureView = wgpuTextureCreateView(Texture, &msaaViewDesc);
    }

    void Deinit()
    {
        if (TextureView) wgpuTextureViewRelease(TextureView);
        if (Texture) wgpuTextureRelease(Texture);

        Texture = nullptr;
        TextureView = nullptr;
    }
};
