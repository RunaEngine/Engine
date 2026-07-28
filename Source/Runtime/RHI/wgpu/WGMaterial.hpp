#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/RHI/wgpu/WGShader.hpp"
#include "Runtime/RHI/wgpu/WGTexture.hpp"
#include "Runtime/RHI/wgpu/WGMipmap.hpp"
#include "Runtime/RHI/Utils.hpp"
#include <dawn/webgpu_cpp.h>


class WGMaterial : Object
{
private:
    wgpu::Device Device;
    wgpu::Queue Queue;

public:
    SharedPtr<WGShader> Shader = nullptr;
    UniquePtr<WGPipeline> Pipeline = nullptr;
    UniquePtr<WGMipmap> MipmapPipeline = nullptr;
    std::vector<SharedPtr<WGTexture>> Textures;

    WGMaterial(wgpu::Device device, wgpu::Queue queue, SharedPtr<WGShader> shader) : Device(device), Queue(queue)
    {
        Shader = shader;
    }

    ~WGMaterial() override
    {
        Deinit();
    }

    void Init(wgpu::SurfaceConfiguration& surfaceConfig, wgpu::BindGroupLayout cameraBindGroupLayout,
              wgpu::BindGroup cameraBindGroup, bool& msaaEnabled, const std::vector<SharedPtr<WGTexture>>& textures = {}, bool useMipmaps = false)
    {
        Textures = textures;
        Pipeline = MakeUnique<WGPipeline>(Device, surfaceConfig, cameraBindGroupLayout, cameraBindGroup, msaaEnabled);
        Pipeline->Init(Shader, Textures);
        if (useMipmaps)
        {
            SharedPtr<WGShader> mipmapShader = MakeShared<WGShader>(Device);
            mipmapShader->Init(GetBaseDir().string() + "Resources/Shaders/Mipmap.wgsl");
            MipmapPipeline = MakeUnique<WGMipmap>(Device, Queue, surfaceConfig, msaaEnabled);
            MipmapPipeline->Init(mipmapShader);
            for (auto& texture : Textures)
                MipmapPipeline->GenerateMipmaps(texture);
        }
    }

    void Deinit()
    {
        Pipeline->Deinit();
        Shader->Deinit();
    }

    void UpdatePipeline()
    {
        Pipeline->Init(Shader, Textures);
    }

private:

};
