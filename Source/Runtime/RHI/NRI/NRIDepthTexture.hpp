// NRIDepthTexture.hpp
#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/Settings.hpp"
#include <NRI.h>
#include <Extensions/NRIHelper.h>
#include <vector>

class NRIDepthTexture : Object
{
public:
    nri::Texture* Texture = nullptr;
    nri::Descriptor* DepthAttachment = nullptr;
    nri::Format Format = nri::Format::D32_SFLOAT_S8_UINT;
    std::vector<nri::Memory*> MemoryAllocations;

    nri::AccessLayoutStage CurrentState = {
        .access = nri::AccessBits::NONE,
        .layout = nri::Layout::UNDEFINED,
        .stages = nri::StageBits::NONE
    };

    bool Create(nri::CoreInterface& core, nri::HelperInterface& helper, nri::Device& device, uint32_t width,
                uint32_t height)
    {
        nri::TextureDesc depthDesc = {};
        depthDesc.type = nri::TextureType::TEXTURE_2D;
        depthDesc.usage = nri::TextureUsageBits::DEPTH_STENCIL_ATTACHMENT;
        depthDesc.format = Format;
        depthDesc.width = width;
        depthDesc.height = height;
        depthDesc.mipNum = 1;
        depthDesc.layerNum = 1;
        depthDesc.sampleNum = (uint8_t)GUserSettings->MSAACount;
        depthDesc.optimizedClearValue.depthStencil = {1.0f, 0};

        if (core.CreateTexture(device, depthDesc, Texture) != nri::Result::SUCCESS)
            return false;

        nri::ResourceGroupDesc groupDesc = {};
        groupDesc.memoryLocation = nri::MemoryLocation::DEVICE;
        groupDesc.textureNum = 1;
        groupDesc.textures = &Texture;

        uint32_t allocationNum = helper.CalculateAllocationNumber(device, groupDesc);
        MemoryAllocations.resize(allocationNum, nullptr);

        if (helper.AllocateAndBindMemory(device, groupDesc, MemoryAllocations.data()) != nri::Result::SUCCESS)
            return false;

        nri::TextureViewDesc viewDesc = {};
        viewDesc.texture = Texture;
        viewDesc.type = nri::TextureView::DEPTH_STENCIL_ATTACHMENT;
        viewDesc.format = Format;
        viewDesc.mipNum = 1;
        viewDesc.layerNum = 1;
        viewDesc.planes = nri::PlaneBits::DEPTH | nri::PlaneBits::STENCIL;

        return core.CreateTextureView(viewDesc, DepthAttachment) == nri::Result::SUCCESS;
    }

    void Deinit(nri::CoreInterface& core)
    {
        if (DepthAttachment) core.DestroyDescriptor(DepthAttachment);
        if (Texture) core.DestroyTexture(Texture);

        for (nri::Memory* memory : MemoryAllocations)
        {
            if (memory) core.FreeMemory(memory);
        }

        MemoryAllocations.clear();
        DepthAttachment = nullptr;
        Texture = nullptr;
    }
};
