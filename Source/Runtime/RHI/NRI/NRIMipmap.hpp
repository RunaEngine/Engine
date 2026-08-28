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

    // Called from the command buffer recording thread, not from the main thread.
    void GenerateMipmaps(nri::CommandBuffer* cmdBuffer, SharedPtr<NRITexture> texture)
    {
        uint32_t mipCount = texture->GetMipLevels();
        if (mipCount <= 1 || !ComputePipeline) return;
        if (mipCount - 1 > MaxMipSteps)
        {
            Logs::Error("NRIMipmap: textura tem mais mips do que o pool suporta");
            return;
        }

        ICore.CmdSetPipelineLayout(*cmdBuffer, nri::BindPoint::COMPUTE, *MipmapLayout);
        ICore.CmdSetPipeline(*cmdBuffer, *ComputePipeline);
        ICore.CmdSetDescriptorPool(*cmdBuffer, *DescriptorPool);

        uint32_t width = texture->GetWidth();
        uint32_t height = texture->GetHeight();

        for (uint32_t mip = 1; mip < mipCount; mip++)
        {
            width = std::max(1u, width / 2);
            height = std::max(1u, height / 2);

            // Barrier: before mip
            // turn to read
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
            ICore.CmdBarrier(*cmdBuffer, barrierDesc);

            nri::DescriptorSet* set = nullptr;
            if (ICore.AllocateDescriptorSets(*DescriptorPool, *MipmapLayout, 0, &set, 1, 0) != nri::Result::SUCCESS)
            {
                Logs::Error("NRIMipmap: failed to allocate descriptor set");
                return;
            }

            nri::Descriptor* srcView = texture->GetStorageView(mip - 1);
            nri::Descriptor* dstView = texture->GetStorageView(mip);
            if (!srcView || !dstView)
            {
                Logs::Error("NRIMipmap: failed to get storage view");
                return;
            }

            nri::UpdateDescriptorRangeDesc updates[2] = {}; 
            updates[0] = { set, 0, 0, &srcView, 1 };
            updates[1] = { set, 1, 0, &dstView, 1 };
            ICore.UpdateDescriptorRanges(updates, 2);

            nri::SetDescriptorSetDesc setBindDesc = {};
            setBindDesc.setIndex = 0;
            setBindDesc.descriptorSet = set;
            ICore.CmdSetDescriptorSet(*cmdBuffer, setBindDesc);

            uint32_t dispatchX = (width + 15) / 16;
            uint32_t dispatchY = (height + 15) / 16;
            ICore.CmdDispatch(*cmdBuffer, { dispatchX, dispatchY, 1 });
        }

        // End barrier
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
        ICore.CmdBarrier(*cmdBuffer, finalBarrierDesc);

        texture->ClearDirty();
    }
};