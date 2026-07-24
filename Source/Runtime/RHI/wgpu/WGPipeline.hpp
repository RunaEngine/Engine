#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/RHI/wgpu/WGVertexBuffer.hpp"
#include "Runtime/RHI/wgpu/WGShader.hpp"
#include "Runtime/RHI/wgpu/WGTexture.hpp"
#include <webgpu/wgpu.h>
#include <cstring>


class WGPipeline : Object
{
private:
    WGPUDevice Device = nullptr;
    WGPUSurfaceConfiguration SurfaceConfig;
    SharedPtr<WGShader> Shader = nullptr;

public:
    WGPUPrimitiveState PrimitiveState = {};
    WGPUColorTargetState ColorTarget = {};
    WGPUBlendState BlendState = {};
    WGPUVertexBufferLayout VertexLayout = {};
    WGPUFragmentState FragmentState = {};
    WGPUPipelineLayout PipelineLayout = nullptr;
    WGPURenderPipeline RenderPipeline = nullptr;

    WGPipeline(WGPUDevice device, WGPUSurfaceConfiguration surfaceConfig) : Device(device), SurfaceConfig(surfaceConfig)
    {
    }

    ~WGPipeline() override
    {
        Deinit();
        Device = nullptr;
    }

    void Init(SharedPtr<WGShader> shader, std::vector<SharedPtr<WGTexture>> textures = {})
    {
        Deinit();
        Shader = shader;

        std::vector<WGPUBindGroupLayout> textureBindGroupLayouts;
        textureBindGroupLayouts.reserve(textures.size());

        for (auto& texture : textures)
        {
            if (texture && texture->TextureBindGroupLayout) {
                textureBindGroupLayouts.push_back(texture->TextureBindGroupLayout);
            }
        }

        WGPUPipelineLayoutDescriptor layoutDesc = {
            .nextInChain = nullptr,
            .bindGroupLayoutCount = textureBindGroupLayouts.size(),
            .bindGroupLayouts = textureBindGroupLayouts.empty() ? nullptr : textureBindGroupLayouts.data(),
            .immediateSize = 0
        };

        PipelineLayout = wgpuDeviceCreatePipelineLayout(Device, &layoutDesc);

        UpdateColorTarget();
        CreateFragmentState(shader);

        WGPURenderPipelineDescriptor renderDesc = {
            .layout = PipelineLayout,
            .vertex = CreateVertexState(Shader),
            .primitive = PrimitiveState,
            //.depthStencil = nullptr,
            .multisample = {
                .count = 1,
                .mask = (uint32_t)~0,
                .alphaToCoverageEnabled = false
            },
            .fragment = &FragmentState,
        };

        RenderPipeline = wgpuDeviceCreateRenderPipeline(Device, &renderDesc);
    }

    void Deinit()
    {
        if (RenderPipeline) wgpuRenderPipelineRelease(RenderPipeline);
        if (PipelineLayout) wgpuPipelineLayoutRelease(PipelineLayout);
    }

    void UpdateColorTarget()
    {
        ColorTarget = {
            .format = SurfaceConfig.format,
            .blend = &BlendState,
            .writeMask = WGPUColorWriteMask_All
        };
    }

private:
    WGPUVertexState CreateVertexState(SharedPtr<WGShader> shader)
    {
        auto layout = WGVertex::GetLayout();
        WGPUVertexState vertexState = {
            .module = shader->Get(),
            .entryPoint = {
                .data = shader->GetVertexEntry(),
                .length = static_cast<uint32_t>(std::strlen(shader->GetVertexEntry()))
            },
            .bufferCount = 1,
            .buffers = &layout
        };
        return vertexState;
    }

    void CreateFragmentState(SharedPtr<WGShader> shader)
    {
        FragmentState = {
            .module = shader->Get(),
            .entryPoint = {
                .data = shader->GetFragmentEntry(),
                .length = static_cast<uint32_t>(std::strlen(shader->GetFragmentEntry()))
            },
            .targetCount = 1,
            .targets = &ColorTarget
        };
    }

    void CreateBlendState()
    {
        BlendState = {
            .color = {
                .operation = WGPUBlendOperation_Add,
                .srcFactor = WGPUBlendFactor_One,
                .dstFactor = WGPUBlendFactor_Zero
            },
            .alpha = {
                .operation = WGPUBlendOperation_Add,
            }
        };
    }
};
