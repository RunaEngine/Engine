#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/RHI/wgpu/WGShader.hpp"
#include "Runtime/RHI/Utils.hpp"
#include <dawn/webgpu_cpp.h>

class WGMipmap : Object
{
private:
    wgpu::Device Device;
    wgpu::Queue Queue;
    wgpu::SurfaceConfiguration& SurfaceConfig;

public:
    wgpu::ColorTargetState ColorTarget = {};
    wgpu::FragmentState FragmentState = {};
    wgpu::PrimitiveState PrimitiveState = {};
    bool& MSAAEnabled;
    wgpu::PipelineLayout MipmapLayout = nullptr;
    wgpu::RenderPipeline Mipmap = nullptr;
    wgpu::Sampler Sampler = nullptr;
    wgpu::ComputePipeline ComputeMipmap = nullptr;
    wgpu::BindGroupLayout MipmapTextureLayout = nullptr;

    WGMipmap(wgpu::Device device, wgpu::Queue queue, wgpu::SurfaceConfiguration& surfaceConfig, bool& msaaEnabled) : Device(device), Queue(queue), SurfaceConfig(surfaceConfig),
                                                 MSAAEnabled(msaaEnabled)
    {
    }

    ~WGMipmap() override
    {
        Deinit();
    }

    void Init(SharedPtr<WGShader> shader)
    {
        Deinit();

        wgpu::BindGroupLayoutEntry bindGroupLayoutEntries[2] = {};
        bindGroupLayoutEntries[0].binding = 0;
        bindGroupLayoutEntries[0].visibility = wgpu::ShaderStage::Fragment;
        bindGroupLayoutEntries[0].texture.sampleType = wgpu::TextureSampleType::Float;
        bindGroupLayoutEntries[0].texture.viewDimension = wgpu::TextureViewDimension::e2D;
        bindGroupLayoutEntries[0].texture.multisampled = false;

        bindGroupLayoutEntries[1].binding = 1;
        bindGroupLayoutEntries[1].visibility = wgpu::ShaderStage::Fragment;
        bindGroupLayoutEntries[1].sampler.type = wgpu::SamplerBindingType::Filtering;

        wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc = {
            .entryCount = 2,
            .entries = bindGroupLayoutEntries,
        };
        MipmapTextureLayout = Device.CreateBindGroupLayout(&bindGroupLayoutDesc);

        wgpu::PipelineLayoutDescriptor layoutDesc = {
            .bindGroupLayoutCount = 1,
            .bindGroupLayouts = &MipmapTextureLayout,
        };

        MipmapLayout = Device.CreatePipelineLayout(&layoutDesc);

        CreateColorTarget();
        CreatePrimitiveState();
        CreateFragmentState(shader);

        wgpu::RenderPipelineDescriptor renderDesc = {
            .layout = MipmapLayout,
            .vertex = CreateVertexState(shader),
            .primitive = PrimitiveState,
            .multisample = {
                .count = MSAAEnabled ? uint8_t(4) : uint8_t(1),
                .mask = (uint32_t)~0,
                .alphaToCoverageEnabled = false
            },
            .fragment = &FragmentState
        };

        Mipmap = Device.CreateRenderPipeline(&renderDesc);

        wgpu::SamplerDescriptor SamplerDesc = {};
        SamplerDesc.minFilter = wgpu::FilterMode::Linear;
        SamplerDesc.magFilter = wgpu::FilterMode::Linear;
        SamplerDesc.mipmapFilter = wgpu::MipmapFilterMode::Linear;
        SamplerDesc.lodMinClamp = 0.0f;
        SamplerDesc.lodMaxClamp = 32.0f;
        Sampler = Device.CreateSampler(&SamplerDesc);
    }

    void Deinit()
    {
        Mipmap = nullptr;
        MipmapLayout = nullptr;
        MipmapTextureLayout = nullptr;
    }

    void GenerateMipmaps(SharedPtr<WGTexture> texture)
    {
        if (texture->Texture.GetFormat() != wgpu::TextureFormat::RGBA8Unorm)
        {
            Logs::Error("Texture format not supported for mipmap generation");
            return;
        }

        if (texture->GetMipLevelCount() == 1)
        {
            return;
        }

        auto encoder = Device.CreateCommandEncoder();

        wgpu::TextureView texView;
        wgpu::Texture tempTexture;
        if (texture->Texture.GetUsage() & wgpu::TextureUsage::RenderAttachment)
        {
            tempTexture = texture->Texture;

            wgpu::TextureViewDescriptor desc = {};
            desc.format = WGPUtils::RemoveSrgbSuffix(texture->Texture.GetFormat());
            desc.baseMipLevel = 0;
            desc.mipLevelCount = 1;

            texView = tempTexture.CreateView(&desc);
        }
        else
        {
            wgpu::TextureDescriptor texDescriptor = {
                .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc,
                .dimension = texture->Texture.GetDimension(),
                .size = texture->GetExtent(),
                .format = WGPUtils::RemoveSrgbSuffix(texture->Texture.GetFormat()),
                .mipLevelCount = texture->GetMipLevelCount(),
                .sampleCount = texture->Texture.GetSampleCount(),
            };
            wgpu::Texture temp = Device.CreateTexture(&texDescriptor);
            wgpu::TexelCopyTextureInfo source = {
                .texture = texture->Texture,
                .mipLevel = 0,
                .origin = {0, 0, 0},
                .aspect = wgpu::TextureAspect::All,
            };

            wgpu::TexelCopyTextureInfo destination = {
                .texture = temp,
                .mipLevel = 0,
                .origin = {0, 0, 0},
                .aspect = wgpu::TextureAspect::All,
            };
            encoder.CopyTextureToTexture(&source, &destination, &texDescriptor.size);

            wgpu::TextureViewDescriptor texViewDescriptor = {};
            texViewDescriptor.mipLevelCount = 1;

            texView = temp.CreateView(&texViewDescriptor);
            tempTexture = temp;
        }

        for (uint32_t mip = 1; mip < texture->GetMipLevelCount(); mip++)
        {
            wgpu::TextureViewDescriptor dstViewDesc = {
                .format = WGPUtils::RemoveSrgbSuffix(texture->Texture.GetFormat()),
                .baseMipLevel = mip,
                .mipLevelCount = 1,
            };
            wgpu::TextureView dstView = tempTexture.CreateView(&dstViewDesc);

            wgpu::BindGroupEntry entries[2] = {
                {
                    .binding = 0,
                    .textureView = texView,
                },
                {
                    .binding = 1,
                    .sampler = Sampler,
                },
            };

            wgpu::BindGroupDescriptor bindGroupDesc = {
                .layout = Mipmap.GetBindGroupLayout(0),
                .entryCount = 2,
                .entries = entries,
            };
            wgpu::BindGroup textureBindGroup = Device.CreateBindGroup(&bindGroupDesc);

            wgpu::RenderPassColorAttachment colorAttachment = {
                .view = dstView,
                .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
                .resolveTarget = nullptr,
                .loadOp = wgpu::LoadOp::Clear,
                .storeOp = wgpu::StoreOp::Store,
            };

            wgpu::RenderPassDescriptor passDesc = {
                .colorAttachmentCount = 1,
                .colorAttachments = &colorAttachment,
                .depthStencilAttachment = nullptr,
                .occlusionQuerySet = nullptr,
                .timestampWrites = nullptr,
            };

            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);
            pass.SetPipeline(Mipmap);
            pass.SetBindGroup(0, textureBindGroup);
            pass.Draw(3);
            pass.End();

            texView = dstView;
        }

        if (tempTexture)
        {
            wgpu::Extent3D size = texture->GetExtent();

            for (uint32_t mipLevel = 0; mipLevel < texture->GetMipLevelCount(); mipLevel++)
            {
                wgpu::TexelCopyTextureInfo source = {
                    .texture = tempTexture,
                    .mipLevel = mipLevel,
                    .origin = {0, 0, 0},
                    .aspect = wgpu::TextureAspect::All,
                };

                wgpu::TexelCopyTextureInfo destination = {
                    .texture = texture->Texture,
                    .mipLevel = mipLevel,
                    .origin = {0, 0, 0},
                    .aspect = wgpu::TextureAspect::All,
                };

                encoder.CopyTextureToTexture(&source, &destination, &size);

                size.width /= 2;
                size.height /= 2;
            }
        }

        wgpu::CommandBuffer cmdBuffer = encoder.Finish();
        Queue.Submit(1, &cmdBuffer);
    }

private:
    void CreateColorTarget()
    {
        ColorTarget = {
            .format = wgpu::TextureFormat::RGBA8Unorm,
        };
    }

    wgpu::VertexState CreateVertexState(SharedPtr<WGShader> shader)
    {
        wgpu::VertexState vertexState = {};
        vertexState.module = shader->Get();
        vertexState.entryPoint.data = shader->GetVertexEntry();
        vertexState.entryPoint.length = static_cast<uint32_t>(std::strlen(shader->GetVertexEntry()));
        vertexState.bufferCount = 0;
        vertexState.buffers = nullptr;

        return vertexState;
    }

    void CreateFragmentState(SharedPtr<WGShader> shader)
    {
        FragmentState = {};
        FragmentState.module = shader->Get(),
        FragmentState.entryPoint.data = shader->GetFragmentEntry(),
        FragmentState.entryPoint.length = static_cast<uint32_t>(std::strlen(shader->GetFragmentEntry())),
        FragmentState.targetCount = 1,
        FragmentState.targets = &ColorTarget;
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
};
