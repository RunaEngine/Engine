#pragma once

#include "Engine/Core/Object.hpp"
#include <NRI.h>
#include <Extensions/NRIHelper.h>
#include <vector>

class NRIMultisampleTexture : Object
{
public:
    nri::Texture* Texture = nullptr;
    nri::Descriptor* ColorAttachment = nullptr;
    nri::Format Format = nri::Format::UNKNOWN;
    std::vector<nri::Memory*> MemoryAllocations;

    nri::AccessLayoutStage CurrentState = {
        nri::AccessBits::NONE,
        nri::Layout::UNDEFINED,
        nri::StageBits::NONE
    };

    bool Create(nri::CoreInterface& core, nri::HelperInterface& helper, nri::Device& device, uint32_t width, uint32_t height, nri::Format format, uint8_t sampleNum)
    {
        Format = format;

        nri::TextureDesc textureDesc = {};
        textureDesc.type = nri::TextureType::TEXTURE_2D;
        textureDesc.usage = nri::TextureUsageBits::COLOR_ATTACHMENT;
        textureDesc.format = Format;
        textureDesc.width = (nri::Dim_t)width;
        textureDesc.height = (nri::Dim_t)height;
        textureDesc.sampleNum = sampleNum;
        textureDesc.mipNum = 1;
        textureDesc.layerNum = 1;

        if (core.CreateTexture(device, textureDesc, Texture) != nri::Result::SUCCESS)
            return false;

        nri::ResourceGroupDesc groupDesc = {};
        groupDesc.memoryLocation = nri::MemoryLocation::DEVICE;
        groupDesc.textureNum = 1;
        groupDesc.textures = &Texture;

        uint32_t allocationNum = helper.CalculateAllocationNumber(device, groupDesc);
        MemoryAllocations.resize(allocationNum, nullptr);

        if (helper.AllocateAndBindMemory(device, groupDesc, MemoryAllocations.data()) != nri::Result::SUCCESS)
            return false;

        nri::TextureViewDesc viewDesc = { Texture, nri::TextureView::COLOR_ATTACHMENT, Format };
        return core.CreateTextureView(viewDesc, ColorAttachment) == nri::Result::SUCCESS;
    }

    void Deinit(nri::CoreInterface& core)
    {
        if (ColorAttachment) core.DestroyDescriptor(ColorAttachment);
        if (Texture) core.DestroyTexture(Texture);

        for (nri::Memory* memory : MemoryAllocations)
        {
            if (memory) core.FreeMemory(memory);
        }

        MemoryAllocations.clear();
        ColorAttachment = nullptr;
        Texture = nullptr;

        CurrentState = { nri::AccessBits::NONE, nri::Layout::UNDEFINED, nri::StageBits::NONE };
    }
};
