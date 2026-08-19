#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/Utils/Logs.hpp"
#include <NRI.h>
#include <Extensions/NRIHelper.h>
#include <vector>

class RHITexture : Object
{
private:
    nri::CoreInterface& ICore;
    nri::HelperInterface& IHelper;
    nri::Device* Device = nullptr;
    bool OwnsTexture = false;
public:
    nri::Texture* Texture = nullptr;
    nri::Descriptor* View = nullptr;
    nri::Format Format = nri::Format::UNKNOWN;
    nri::Dim_t Width = 0;
    nri::Dim_t Height = 0;

    RHITexture(nri::CoreInterface& core, nri::HelperInterface& helper, nri::Device* device) : ICore(core), IHelper(helper), Device(device) {}
    ~RHITexture() override { Deinit(); }

    bool Init(const nri::TextureDesc& desc, nri::MemoryLocation memoryLocation, std::vector<nri::Memory*>& outMemory)
    {
        nri::Result result = ICore.CreateTexture(*Device, desc, Texture);
        if (result != nri::Result::SUCCESS)
        {
            Logs::Error("RHITexture: CreateTexture failed: %d", (int)result);
            return false;
        }

        nri::ResourceGroupDesc groupDesc = {};
        groupDesc.memoryLocation = memoryLocation;
        groupDesc.textureNum = 1;
        groupDesc.textures = &Texture;

        size_t startIndex = outMemory.size();
        uint32_t allocationNum = IHelper.CalculateAllocationNumber(*Device, groupDesc);
        outMemory.resize(startIndex + allocationNum, nullptr);

        result = IHelper.AllocateAndBindMemory(*Device, groupDesc, outMemory.data() + startIndex);
        if (result != nri::Result::SUCCESS)
        {
            Logs::Error("RHITexture: AllocateAndBindMemory failed: %d", (int)result);
            return false;
        }

        Format = desc.format;
        Width = desc.width;
        Height = desc.height;
        OwnsTexture = true;
        return true;
    }

    void WrapExisting(nri::Texture* texture, nri::Format format, nri::Dim_t width, nri::Dim_t height)
    {
        Texture = texture;
        Format = format;
        Width = width;
        Height = height;
        OwnsTexture = false;
    }

    bool CreateView(nri::TextureView viewType, nri::PlaneBits planes = nri::PlaneBits::ALL)
    {
        nri::TextureViewDesc viewDesc = {};
        viewDesc.texture = Texture;
        viewDesc.type = viewType;
        viewDesc.format = Format;
        viewDesc.mipNum = 1;
        viewDesc.layerNum = 1;
        viewDesc.planes = planes;

        nri::Result result = ICore.CreateTextureView(viewDesc, View);
        if (result != nri::Result::SUCCESS)
        {
            Logs::Error("RHITexture: CreateTextureView failed: %d", (int)result);
            return false;
        }
        return true;
    }

    void Deinit()
    {
        if (View)
        {
            ICore.DestroyDescriptor(View);
            View = nullptr;
        }
        if (Texture && OwnsTexture)
        {
            ICore.DestroyTexture(Texture);
        }
        Texture = nullptr;
    }
};
