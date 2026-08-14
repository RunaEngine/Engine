#pragma once

#include "Engine/Core/Object.hpp"
#include <dawn/webgpu_cpp.h>

class WGMultisample : Object
{
public:
    wgpu::Texture Texture = nullptr;
    wgpu::TextureView TextureView = nullptr;

    WGMultisample() = default;
    ~WGMultisample() override = default;

    void Init(wgpu::Device device, wgpu::SurfaceConfiguration& surfaceConfig)
    {
        Deinit();

        wgpu::Extent3D extend = {
            .width = surfaceConfig.width,
            .height = surfaceConfig.height,
            .depthOrArrayLayers = 1
        };

        wgpu::TextureDescriptor msaaDesc = {
            .label = WGPUtils::StrToWgpuStringView("MSAATexture"),
            .usage = wgpu::TextureUsage::RenderAttachment,
            .dimension = wgpu::TextureDimension::e2D,
            .size = extend,
            .format = surfaceConfig.format,
            .mipLevelCount = 1,
            .sampleCount = 4
        };

        Texture = device.CreateTexture(&msaaDesc);

        wgpu::TextureViewDescriptor msaaViewDesc = {
            .format = surfaceConfig.format,
            .dimension = wgpu::TextureViewDimension::e2D,
            .baseMipLevel = 0,
            .mipLevelCount = 1,
            .baseArrayLayer = 0,
            .arrayLayerCount = 1,
        };
        TextureView = Texture.CreateView(&msaaViewDesc);
    }

    void Deinit()
    {
        TextureView = nullptr;
        Texture = nullptr;
    }
};
