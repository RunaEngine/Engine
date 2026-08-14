#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/Settings.hpp"
#include "Runtime/RHI/wgpu/WGVertexBuffer.hpp"
#include "Runtime/RHI/wgpu/WGShader.hpp"
#include "Runtime/RHI/wgpu/WGTexture.hpp"
#include <dawn/webgpu_cpp.h>


class WGPipeline : Object
{
private:
    wgpu::Device Device;
    wgpu::SurfaceConfiguration& SurfaceConfig;

public:
    wgpu::PrimitiveState PrimitiveState = {};
    wgpu::ColorTargetState ColorTarget = {};
    wgpu::BlendState BlendState = {};
    wgpu::VertexBufferLayout VertexLayout = {};
    wgpu::FragmentState FragmentState = {};
    wgpu::DepthStencilState DepthStencil = {};
    wgpu::BindGroupLayout CameraBindGroupLayout;
    wgpu::BindGroup CameraBindGroup;
    wgpu::PipelineLayout PipelineLayout = nullptr;
    wgpu::RenderPipeline RenderPipeline = nullptr;

    WGPipeline(wgpu::Device device, wgpu::SurfaceConfiguration& surfaceConfig, wgpu::BindGroupLayout cameraBindGroupLayout,
               wgpu::BindGroup cameraBindGroup) : Device(device), SurfaceConfig(surfaceConfig),
                                                 CameraBindGroupLayout(cameraBindGroupLayout),
                                                 CameraBindGroup(cameraBindGroup)
    {
    }

    ~WGPipeline() override
    {
        Deinit();
    }

    void Init(SharedPtr<WGShader> shader, std::vector<SharedPtr<WGTexture>> textures = {})
    {
        Deinit();

        std::vector<wgpu::BindGroupLayout> pipelineBindGroupLayouts;
        pipelineBindGroupLayouts.reserve(textures.size() + 1);

        for (auto& texture : textures)
        {
            if (texture && texture->TextureBindGroupLayout)
            {
                pipelineBindGroupLayouts.push_back(texture->TextureBindGroupLayout);
            }
        }
        pipelineBindGroupLayouts.push_back(CameraBindGroupLayout);

        wgpu::PipelineLayoutDescriptor layoutDesc = {
            .nextInChain = nullptr,
            .label = WGPUtils::StrToWgpuStringView("PipelineLayout"),
            .bindGroupLayoutCount = pipelineBindGroupLayouts.size(),
            .bindGroupLayouts = pipelineBindGroupLayouts.empty() ? nullptr : pipelineBindGroupLayouts.data(),
            .immediateSize = 0,
        };

        PipelineLayout = Device.CreatePipelineLayout(&layoutDesc);

        CreateBlendState();
        CreateColorTarget();
        CreatePrimitiveState();
        CreateDepthStencil();
        CreateFragmentState(shader);

        wgpu::RenderPipelineDescriptor renderDesc = {
            .layout = PipelineLayout,
            .vertex = CreateVertexState(shader),
            .primitive = PrimitiveState,
            .depthStencil = &DepthStencil,
            .multisample = {
                .count = GUserSettings->bMSAAEnabled ? uint8_t(4) : uint8_t(1),
                .mask = (uint32_t)~0,
                .alphaToCoverageEnabled = false
            },
            .fragment = &FragmentState,
        };

        RenderPipeline = Device.CreateRenderPipeline(&renderDesc);
    }

    void Deinit()
    {
        RenderPipeline = nullptr;
        PipelineLayout = nullptr;
    }

private:
    void CreateColorTarget()
    {
        ColorTarget = {
            .format = SurfaceConfig.format,
            .blend = &BlendState,
            .writeMask = wgpu::ColorWriteMask::All
        };
    }

    wgpu::VertexState CreateVertexState(SharedPtr<WGShader> shader)
    {
        VertexLayout = WGVertex::GetLayout();

        wgpu::VertexState vertexState = {};
        vertexState.module = shader->Get(),
        vertexState.entryPoint = WGPUtils::StrToWgpuStringView(shader->GetVertexEntry()),
        vertexState.bufferCount = 1,
        vertexState.buffers = &VertexLayout;

        return vertexState;
    }

    void CreateFragmentState(SharedPtr<WGShader> shader)
    {
        FragmentState = {};
        FragmentState.module = shader->Get(),
        FragmentState.entryPoint = WGPUtils::StrToWgpuStringView(shader->GetFragmentEntry()),
        FragmentState.targetCount = 1,
        FragmentState.targets = &ColorTarget;
    }

    void CreateBlendState()
    {
        BlendState = {
            .color = {
                .operation = wgpu::BlendOperation::Add,
                .srcFactor = wgpu::BlendFactor::One,
                .dstFactor = wgpu::BlendFactor::Zero
            },
            .alpha = {
                .operation = wgpu::BlendOperation::Add,
            }
        };
    }

    void CreatePrimitiveState()
    {
        PrimitiveState = {
            .topology = wgpu::PrimitiveTopology::TriangleList,
            .frontFace = wgpu::FrontFace::CCW,
            .cullMode = wgpu::CullMode::Back,
            .unclippedDepth = false,
        };
    }

    void CreateDepthStencil()
    {
        DepthStencil = {
            .format = wgpu::TextureFormat::Depth32FloatStencil8,
            .depthWriteEnabled = wgpu::OptionalBool::True,
            .depthCompare = wgpu::CompareFunction::Less
        };
    }
};
