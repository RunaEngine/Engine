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
    WGPUSurfaceConfiguration& SurfaceConfig;
    
public:
    WGPUPrimitiveState PrimitiveState = {};
    WGPUColorTargetState ColorTarget = {};
    WGPUBlendState BlendState = {};
    WGPUVertexBufferLayout VertexLayout = {};
    WGPUFragmentState FragmentState = {};
    WGPUDepthStencilState DepthStencil = {};
    WGPUBindGroupLayout& CameraBindGroupLayout;
    WGPUBindGroup& CameraBindGroup;
    WGPUPipelineLayout PipelineLayout = nullptr;
    WGPURenderPipeline RenderPipeline = nullptr;

    WGPipeline(WGPUDevice device, WGPUSurfaceConfiguration& surfaceConfig, WGPUBindGroupLayout& cameraBindGroupLayout, WGPUBindGroup& cameraBindGroup) : Device(device), SurfaceConfig(surfaceConfig), CameraBindGroupLayout(cameraBindGroupLayout), CameraBindGroup(cameraBindGroup)
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

        std::vector<WGPUBindGroupLayout> pipelineBindGroupLayouts;
        pipelineBindGroupLayouts.reserve(textures.size() + 1);

        for (auto& texture : textures)
        {
            if (texture && texture->TextureBindGroupLayout) {
                pipelineBindGroupLayouts.push_back(texture->TextureBindGroupLayout);
            }
        }
        pipelineBindGroupLayouts.push_back(CameraBindGroupLayout);

        WGPUPipelineLayoutDescriptor layoutDesc = {
            .nextInChain = nullptr,
            .bindGroupLayoutCount = pipelineBindGroupLayouts.size(),
            .bindGroupLayouts = pipelineBindGroupLayouts.empty() ? nullptr : pipelineBindGroupLayouts.data(),
            .immediateSize = 0
        };

        PipelineLayout = wgpuDeviceCreatePipelineLayout(Device, &layoutDesc);

        CreateBlendState();
        CreateColorTarget();
        CreatePrimitiveState();
        CreateDepthStencil();
        CreateFragmentState(shader);

        WGPURenderPipelineDescriptor renderDesc = {
            .layout = PipelineLayout,
            .vertex = CreateVertexState(shader),
            .primitive = PrimitiveState,
            .depthStencil = &DepthStencil,
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

private:
    void CreateColorTarget()
    {
        ColorTarget = {
            .format = SurfaceConfig.format,
            .blend = &BlendState,
            .writeMask = WGPUColorWriteMask_All
        };
    }

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

    void CreatePrimitiveState()
    {
        
        WGPUPrimitiveStateExtras primitiveExtra = {
            .polygonMode = WGPUPolygonMode_Fill,
            .conservative = false
        };
        PrimitiveState = {
            .nextInChain = (WGPUChainedStruct*)&primitiveExtra,
            .topology = WGPUPrimitiveTopology_TriangleList,
            .frontFace = WGPUFrontFace_CCW,
            .cullMode = WGPUCullMode_Back,
            .unclippedDepth = false,
        };
    }

    void CreateDepthStencil()
    {
        DepthStencil = {
            .format = WGPUTextureFormat_Depth32FloatStencil8,
            .depthWriteEnabled = WGPUOptionalBool_True,
            .depthCompare = WGPUCompareFunction_Less
        };
    }
};
