#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/RHI/wgpu/WGShader.hpp"
#include "Runtime/RHI/wgpu/WGTexture.hpp"
#include "Runtime/Utils/Logs.hpp"
#include "Runtime/Utils/System.hpp"
#include <webgpu/wgpu.h>


class WGMaterial : Object
{
private:
    WGPUDevice Device = nullptr;

public:
    SharedPtr<WGShader> Shader = nullptr;
    UniquePtr<WGPipeline> Pipeline = nullptr;
    std::vector<SharedPtr<WGTexture>> Textures;

    WGMaterial(WGPUDevice device, SharedPtr<WGShader> shader) : Device(device), Shader(shader)
    {
    }

    ~WGMaterial() override
    {
        Deinit();
    }

    void Init(WGPUSurfaceConfiguration surfaceConfig, SharedPtr<WGCamera> camera, const std::vector<SharedPtr<WGTexture>>& textures = {})
    {
        Textures = textures;
        Pipeline = MakeUnique<WGPipeline>(Device, surfaceConfig);
        Pipeline->Init(Shader, camera, Textures);
    }

    void Deinit()
    {
        Pipeline->Deinit();
        Shader->Deinit();
    }
};
