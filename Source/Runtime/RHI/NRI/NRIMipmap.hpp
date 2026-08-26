#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/RHI/NRI/NRITexture.hpp"
#include "Runtime/RHI/NRI/NRIShader.hpp"
#include <NRI.h>
#include <algorithm>

class NRIMipmap : public Object
{
private:
    nri::CoreInterface& ICore;
    nri::Device* Device = nullptr;

    nri::PipelineLayout* MipmapLayout = nullptr;
    nri::Pipeline* ComputePipeline = nullptr;
    nri::DescriptorPool* DescriptorPool = nullptr;

    static constexpr uint32_t MaxMipSteps = 16;

public:
    NRIMipmap(nri::CoreInterface& core, nri::Device* device)
        : ICore(core), Device(device)
    {
    }

    ~NRIMipmap() override { Deinit(); }

    bool Init(SharedPtr<NRIShader> computeShader)
    {
        Deinit();

        if (computeShader->ShaderDesc.size == 0)
        {
            Logs::Error("NRIMipmap: shader has no compute entry point (cs_main)");
            return false;
        }

        // Binding 0 -> SrcMip (u0), Binding 1 -> DstMip (u1), space0
        nri::DescriptorRangeDesc ranges[2] = {};
        ranges[0].baseRegisterIndex = 0;
        ranges[0].descriptorNum = 1;
        ranges[0].descriptorType = nri::DescriptorType::STORAGE_TEXTURE; // ⚠️ confira o nome exato
        ranges[0].shaderStages = nri::StageBits::COMPUTE_SHADER;

        ranges[1].baseRegisterIndex = 1;
        ranges[1].descriptorNum = 1;
        ranges[1].descriptorType = nri::DescriptorType::STORAGE_TEXTURE;
        ranges[1].shaderStages = nri::StageBits::COMPUTE_SHADER;

        nri::DescriptorSetDesc setDesc = {};
        setDesc.registerSpace = 0;
        setDesc.ranges = ranges;
        setDesc.rangeNum = 2;

        nri::PipelineLayoutDesc layoutDesc = {};
        layoutDesc.descriptorSets = &setDesc;
        layoutDesc.descriptorSetNum = 1;
        layoutDesc.shaderStages = nri::StageBits::COMPUTE_SHADER;

        if (ICore.CreatePipelineLayout(*Device, layoutDesc, MipmapLayout) != nri::Result::SUCCESS)
        {
            Logs::Error("NRIMipmap: failed to create pipeline layout");
            return false;
        }

        nri::ComputePipelineDesc pipelineDesc = {};
        pipelineDesc.pipelineLayout = MipmapLayout;
        pipelineDesc.shader = computeShader->ShaderDesc;

        if (ICore.CreateComputePipeline(*Device, pipelineDesc, ComputePipeline) != nri::Result::SUCCESS)
        {
            Logs::Error("NRIMipmap: failed to create compute pipeline");
            return false;
        }

        nri::DescriptorPoolDesc poolDesc = {};
        poolDesc.descriptorSetMaxNum = MaxMipSteps;
        poolDesc.storageTextureMaxNum = MaxMipSteps * 2;

        if (ICore.CreateDescriptorPool(*Device, poolDesc, DescriptorPool) != nri::Result::SUCCESS)
        {
            Logs::Error("NRIMipmap: failed to create descriptor pool");
            return false;
        }

        return true;
    }

    void Deinit()
    {
        if (DescriptorPool) { ICore.DestroyDescriptorPool(DescriptorPool); DescriptorPool = nullptr; }
        if (ComputePipeline) { ICore.DestroyPipeline(ComputePipeline); ComputePipeline = nullptr; }
        if (MipmapLayout) { ICore.DestroyPipelineLayout(MipmapLayout); MipmapLayout = nullptr; }
    }

    // Chamar DENTRO do command buffer do frame, logo depois de
    // IStreamer.CmdCopyStreamedData(...) — precisa que o mip 0 já esteja
    // fisicamente copiado na GPU antes do compute rodar.
    void GenerateMipmaps(nri::CommandBuffer& cmdBuffer, SharedPtr<NRITexture> texture)
    {
        uint32_t mipCount = texture->GetMipLevels();
        if (mipCount <= 1 || !ComputePipeline) return;
        if (mipCount - 1 > MaxMipSteps)
        {
            Logs::Error("NRIMipmap: textura tem mais mips do que o pool suporta");
            return;
        }

        ICore.CmdSetPipelineLayout(cmdBuffer, nri::BindPoint::COMPUTE, *MipmapLayout);
        ICore.CmdSetPipeline(cmdBuffer, *ComputePipeline);
        ICore.CmdSetDescriptorPool(cmdBuffer, *DescriptorPool);

        nri::Descriptor* srcView = CreateStorageView(texture, 0);

        uint32_t width = texture->GetWidth();
        uint32_t height = texture->GetHeight();

        nri::DescriptorSet* set = nullptr;
        ICore.AllocateDescriptorSets(*DescriptorPool, *MipmapLayout, 0, &set, 1, 0);
        for (uint32_t mip = 1; mip < mipCount; mip++)
        {
            width = std::max(1u, width / 2);
            height = std::max(1u, height / 2);

            nri::Descriptor* dstView = CreateStorageView(texture, mip);

            // Barrier: mip anterior (recém escrito, ou recém copiado no caso do mip0)
            // vira leitura; mip atual vira escrita.
            nri::TextureBarrierDesc barriers[2] = {};
            barriers[0].texture = texture->Texture;
            barriers[0].before = (mip == 1)
                ? nri::AccessLayoutStage{ nri::AccessBits::COPY_DESTINATION, nri::Layout::COPY_DESTINATION, nri::StageBits::COPY }
                : nri::AccessLayoutStage{ nri::AccessBits::SHADER_RESOURCE_STORAGE, nri::Layout::SHADER_RESOURCE_STORAGE, nri::StageBits::COMPUTE_SHADER };
            barriers[0].after = { nri::AccessBits::SHADER_RESOURCE_STORAGE, nri::Layout::SHADER_RESOURCE_STORAGE, nri::StageBits::COMPUTE_SHADER };
            barriers[0].mipOffset = mip - 1;
            barriers[0].mipNum = 1;
            barriers[0].layerNum = 1;

            barriers[1].texture = texture->Texture;
            barriers[1].before = { nri::AccessBits::NONE, nri::Layout::UNDEFINED, nri::StageBits::NONE };
            barriers[1].after = { nri::AccessBits::SHADER_RESOURCE_STORAGE, nri::Layout::SHADER_RESOURCE_STORAGE, nri::StageBits::COMPUTE_SHADER };
            barriers[1].mipOffset = mip;
            barriers[1].mipNum = 1;
            barriers[1].layerNum = 1;

            nri::BarrierDesc barrierDesc = {};
            barrierDesc.textures = barriers;
            barrierDesc.textureNum = 2;
            ICore.CmdBarrier(cmdBuffer, barrierDesc);

            //nri::DescriptorSet* set = nullptr;
            //ICore.AllocateDescriptorSets(*DescriptorPool, *MipmapLayout, 0, &set, 1, 0);

            nri::UpdateDescriptorRangeDesc updates[2] = {};
            updates[0] = { set, 0, 0, &srcView, 1 };
            updates[1] = { set, 1, 0, &dstView, 1 };
            ICore.UpdateDescriptorRanges(updates, 2);

            nri::SetDescriptorSetDesc setBindDesc = {};
            setBindDesc.setIndex = 0;
            setBindDesc.descriptorSet = set;
            ICore.CmdSetDescriptorSet(cmdBuffer, setBindDesc);

            uint32_t dispatchX = (width + 15) / 16;
            uint32_t dispatchY = (height + 15) / 16;
            ICore.CmdDispatch(cmdBuffer, { dispatchX, dispatchY, 1 });

            ICore.DestroyDescriptor(srcView);
            srcView = dstView;
        }

        ICore.DestroyDescriptor(srcView);

        // Barrier final: cadeia inteira de mips volta pro estado normal de leitura (sampler)
        nri::TextureBarrierDesc finalBarrier = {};
        finalBarrier.texture = texture->Texture;
        finalBarrier.before = { nri::AccessBits::SHADER_RESOURCE_STORAGE, nri::Layout::SHADER_RESOURCE_STORAGE, nri::StageBits::COMPUTE_SHADER };
        finalBarrier.after = { nri::AccessBits::SHADER_RESOURCE, nri::Layout::SHADER_RESOURCE, nri::StageBits::FRAGMENT_SHADER };
        finalBarrier.mipOffset = 0;
        finalBarrier.mipNum = mipCount;
        finalBarrier.layerNum = 1;

        nri::BarrierDesc finalBarrierDesc = {};
        finalBarrierDesc.textures = &finalBarrier;
        finalBarrierDesc.textureNum = 1;
        ICore.CmdBarrier(cmdBuffer, finalBarrierDesc);

        texture->ClearDirty();
    }

private:
    nri::Descriptor* CreateStorageView(SharedPtr<NRITexture> texture, uint32_t mip)
    {
        nri::TextureViewDesc viewDesc = {};
        viewDesc.texture = texture->Texture;
        viewDesc.type = nri::TextureView::STORAGE_TEXTURE;
        viewDesc.format = nri::Format::RGBA8_UNORM;
        viewDesc.mipOffset = mip;
        viewDesc.mipNum = 1;
        viewDesc.layerNum = 1;

        nri::Descriptor* view = nullptr;
        ICore.CreateTextureView(viewDesc, view);
        return view;
    }
};