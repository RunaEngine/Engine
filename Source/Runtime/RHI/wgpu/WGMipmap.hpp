#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/RHI/wgpu/WGShader.hpp"
#include "Runtime/RHI/Utils.hpp"
#include "Runtime/Settings.hpp"
#include <dawn/webgpu_cpp.h>

class WGMipmap : Object
{
private:
    wgpu::Device Device;
    wgpu::Queue Queue;

public:
    wgpu::PipelineLayout MipmapLayout = nullptr;
    wgpu::Sampler Sampler = nullptr;
    wgpu::ComputePipeline ComputeMipmap = nullptr;
    wgpu::BindGroupLayout MipmapTextureLayout = nullptr;

    WGMipmap(wgpu::Device device, wgpu::Queue queue) : Device(device), Queue(queue)
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
        bindGroupLayoutEntries[0].visibility = wgpu::ShaderStage::Compute;
		bindGroupLayoutEntries[0].storageTexture.access = wgpu::StorageTextureAccess::ReadOnly;
		bindGroupLayoutEntries[0].storageTexture.format = wgpu::TextureFormat::RGBA8Unorm;
		bindGroupLayoutEntries[0].storageTexture.viewDimension = wgpu::TextureViewDimension::e2D;

        bindGroupLayoutEntries[1].binding = 1;
        bindGroupLayoutEntries[1].visibility = wgpu::ShaderStage::Compute;
        bindGroupLayoutEntries[1].storageTexture.access = wgpu::StorageTextureAccess::WriteOnly;
        bindGroupLayoutEntries[1].storageTexture.format = wgpu::TextureFormat::RGBA8Unorm;
        bindGroupLayoutEntries[1].storageTexture.viewDimension = wgpu::TextureViewDimension::e2D;

        wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc = {
			.label = WGPUtils::StrToWgpuStringView("MipmapTextureLayout"),
            .entryCount = sizeof(bindGroupLayoutEntries) / sizeof(wgpu::BindGroupLayoutEntry),
            .entries = bindGroupLayoutEntries,
        };
        MipmapTextureLayout = Device.CreateBindGroupLayout(&bindGroupLayoutDesc);

        wgpu::PipelineLayoutDescriptor layoutDesc = {
            .bindGroupLayoutCount = 1,
            .bindGroupLayouts = &MipmapTextureLayout,
        };

        MipmapLayout = Device.CreatePipelineLayout(&layoutDesc);
        wgpu::ComputePipelineDescriptor computeDesc = {
			.label = WGPUtils::StrToWgpuStringView("ComputeMipmap"),
            .layout = MipmapLayout,
			.compute = {
				.module = shader->Get(),
				.entryPoint = WGPUtils::StrToWgpuStringView(shader->GetComputeEntry()),
			},
        };

        ComputeMipmap = Device.CreateComputePipeline(&computeDesc);

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
        ComputeMipmap = nullptr;
        MipmapLayout = nullptr;
        MipmapTextureLayout = nullptr;
    }

    void GenerateMipmaps(SharedPtr<WGTexture> texture)
    {
        if (texture->Texture.GetFormat() != wgpu::TextureFormat::RGBA8Unorm && texture->Texture.GetFormat() != wgpu::TextureFormat::RGBA8UnormSrgb)
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
        if (texture->Texture.GetUsage() & wgpu::TextureUsage::StorageBinding)
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
				.label = WGPUtils::StrToWgpuStringView("TempTextureForMipmap"),
                .usage = wgpu::TextureUsage::StorageBinding | wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc,
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

		wgpu::Extent3D extent = texture->GetExtent();
        uint32_t dispatchX = (extent.width + 15) / 16;
        uint32_t dispatchY = (extent.height + 15) / 16;

        auto pass = encoder.BeginComputePass();
        pass.SetPipeline(ComputeMipmap);

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
					.textureView = dstView,
				},
			};
			wgpu::BindGroupDescriptor bindGroupDesc = {
				.layout = MipmapTextureLayout,
				.entryCount = 2,
				.entries = entries,
			};
			wgpu::BindGroup textureBindGroup = Device.CreateBindGroup(&bindGroupDesc);
			pass.SetBindGroup(0, textureBindGroup);
			pass.DispatchWorkgroups(dispatchX, dispatchY, 1);
			texView = dstView;
		}

        pass.End();

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
};
