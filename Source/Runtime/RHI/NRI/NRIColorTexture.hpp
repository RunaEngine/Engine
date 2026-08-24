#pragma once

#include "Engine/Core/Object.hpp"
#include <NRI.h>

class NRIColorTexture : Object
{
public:
    nri::Texture* Texture = nullptr;
    nri::Descriptor* ColorAttachment = nullptr;
    nri::Format Format = nri::Format::UNKNOWN;

    nri::AccessLayoutStage CurrentState = {
        nri::AccessBits::NONE,
        nri::Layout::UNDEFINED,
        nri::StageBits::NONE
    };

    NRIColorTexture() = default;

    bool InitFromExisting(nri::CoreInterface& core, nri::Texture* existingTexture, nri::Format format)
    {
        Texture = existingTexture;
        Format = format;

        nri::TextureViewDesc viewDesc = { Texture, nri::TextureView::COLOR_ATTACHMENT, Format };
        return core.CreateTextureView(viewDesc, ColorAttachment) == nri::Result::SUCCESS;
    }

    void Deinit(nri::CoreInterface& core)
    {
        if (ColorAttachment)
        {
            core.DestroyDescriptor(ColorAttachment);
            ColorAttachment = nullptr;
        }
        Texture = nullptr;
    }
};
