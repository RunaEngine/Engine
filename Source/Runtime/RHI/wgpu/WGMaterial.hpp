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
    wgpu::SurfaceConfiguration& SurfaceConfig;

    inline static UniquePtr<WGMipmap> MipmapGenerator = nullptr;
public:
    SharedPtr<WGShader> Shader = nullptr;
    UniquePtr<WGPipeline> Pipeline = nullptr;
    UniquePtr<WGMipmap> MipmapPipeline = nullptr;
    std::vector<SharedPtr<WGTexture>> Textures;

    WGMaterial(wgpu::Device device, wgpu::Queue queue, wgpu::SurfaceConfiguration& surfaceConfig, SharedPtr<WGShader> shader) : Device(device), Queue(queue), SurfaceConfig(surfaceConfig)
    {
        Shader = shader;
        if (!MipmapGenerator)
        {
            SharedPtr<WGShader> mipmapShader = MakeShared<WGShader>(Device);
            mipmapShader->Init(GetBaseDir().string() + "Resources/Shaders/Mipmap.wgsl");
            MipmapPipeline = MakeUnique<WGMipmap>(Device, Queue);
            MipmapPipeline->Init(mipmapShader);
        }
    }

    ~WGMaterial() override
    {
        Deinit();
    }

    void Init(wgpu::BindGroupLayout cameraBindGroupLayout,
              wgpu::BindGroup cameraBindGroup, const std::vector<SharedPtr<WGTexture>>& textures = {})
    {
        Textures = textures;
        Pipeline = MakeUnique<WGPipeline>(Device, SurfaceConfig, cameraBindGroupLayout, cameraBindGroup);
        Pipeline->Init(Shader, Textures);
        for (auto& texture : Textures)
        {
            if (texture->GetMipLevelCount() > 0)
            {
                MipmapPipeline->GenerateMipmaps(texture);
            }
        }
    }

    void Deinit()
    {
        Pipeline->Deinit();
        Shader->Deinit();
    }

    void UpdatePipeline()
    {
        for (auto& texture : Textures)
        {
            texture->UpdateSampler();
        }
        Pipeline->Init(Shader, Textures);
    }

private:

};
