#pragma once
#include "Engine/Core/Object.hpp"
#include "Runtime/Utils/Logs.hpp"
#include "Runtime/Settings.hpp"
#include <NRI.h>
#include <Extensions/NRIStreamer.h>
#include <Extensions/NRIHelper.h>
#include <stb_image.h>
#include <stb_image_resize2.h>
#include <filesystem>
#include <vector>

class NRITexture : public Object
{
private:
    nri::CoreInterface& ICore;
    nri::StreamerInterface& IStreamer;
    nri::HelperInterface& IHelper;
    nri::Device* Device = nullptr;
    nri::Streamer* Streamer = nullptr;
    std::vector<nri::Memory*> Memory;

    int TexWidth = 0, TexHeight = 0;
    uint32_t MipLevels = 1;
    nri::AddressMode AddrMode = nri::AddressMode::REPEAT;
    nri::Filter FilterMode = nri::Filter::LINEAR;

    bool bIsDirty = false;

public:
    nri::Texture* Texture = nullptr;
    nri::Descriptor* TextureView = nullptr;
    nri::Descriptor* Sampler = nullptr;

    NRITexture(nri::CoreInterface& core, nri::HelperInterface& helper,
        nri::StreamerInterface& istreamer, nri::Streamer* streamer, nri::Device* device)
        : ICore(core), IHelper(helper), IStreamer(istreamer), Streamer(streamer), Device(device)
    {
    }

    ~NRITexture() override { Deinit(); }

    bool Init(const std::filesystem::path& filepath,
        nri::AddressMode addressMode = nri::AddressMode::REPEAT,
        nri::Filter filterMode = nri::Filter::LINEAR,
        bool generateMipmaps = false)
    {
        Deinit();

        int channels;
        stbi_uc* pixels = stbi_load(filepath.string().c_str(), &TexWidth, &TexHeight, &channels, STBI_rgb_alpha);
        if (!pixels)
        {
            Logs::Error("NRITexture: failed to load '%s'", filepath.string().c_str());
            return false;
        }

        AddrMode = addressMode;
        FilterMode = filterMode;
        MipLevels = generateMipmaps
            ? (uint32_t)(std::floor(std::log2(std::max(TexWidth, TexHeight)))) + 1
            : 1;

        // Create texture
        nri::TextureDesc texDesc = {};
        texDesc.type = nri::TextureType::TEXTURE_2D;
        texDesc.usage = nri::TextureUsageBits::SHADER_RESOURCE;
        if (generateMipmaps)
            texDesc.usage |= nri::TextureUsageBits::SHADER_RESOURCE_STORAGE;
        texDesc.format = nri::Format::RGBA8_UNORM;
        texDesc.width = (nri::Dim_t)TexWidth;
        texDesc.height = (nri::Dim_t)TexHeight;
        texDesc.mipNum = (nri::Dim_t)MipLevels;
        texDesc.layerNum = 1;
        texDesc.sampleNum = 1;

        if (ICore.CreateTexture(*Device, texDesc, Texture) != nri::Result::SUCCESS)
        {
            stbi_image_free(pixels);
            Deinit();
            return false;
        }

		// Allocate memory for the texture and bind it
        nri::ResourceGroupDesc groupDesc = {};
        groupDesc.memoryLocation = nri::MemoryLocation::DEVICE;
        groupDesc.textureNum = 1;
        groupDesc.textures = &Texture;
        Memory.resize(IHelper.CalculateAllocationNumber(*Device, groupDesc), nullptr);
        if (IHelper.AllocateAndBindMemory(*Device, groupDesc, Memory.data()) != nri::Result::SUCCESS)
        {
            stbi_image_free(pixels);
            Deinit();
            return false;
        }

		// Stream texture data to the GPU
        if (!UploadMips(pixels))
        {
            stbi_image_free(pixels);
            Deinit();
            return false;
        }

        stbi_image_free(pixels);

        // 4. View
        nri::TextureViewDesc viewDesc = {};
        viewDesc.texture = Texture;
        viewDesc.type = nri::TextureView::TEXTURE;
        viewDesc.format = nri::Format::RGBA8_UNORM;
        viewDesc.mipNum = (nri::Dim_t)MipLevels;
        viewDesc.layerNum = 1;
        if (ICore.CreateTextureView(viewDesc, TextureView) != nri::Result::SUCCESS)
        {
            Deinit();
            return false;
        }

        // 5. Sampler
        if (!CreateSampler())
        {
            Deinit();
            return false;
        }

        return true;
    }

    void UpdateSampler()
    {
        if (Sampler) { ICore.DestroyDescriptor(Sampler); Sampler = nullptr; }
        CreateSampler();
    }

    void Barrier(nri::CommandBuffer& cmdBuffer)
    {
        if (!bIsDirty) return;
        if (MipLevels > 1) return;

        nri::TextureBarrierDesc barrier = {};
        barrier.texture = Texture;
        barrier.mipOffset = 0;
        barrier.mipNum = (nri::Dim_t)MipLevels;
        barrier.layerOffset = 0;
        barrier.layerNum = 1;

        barrier.before = {
            nri::AccessBits::COPY_DESTINATION,  // ← estava NONE
            nri::Layout::COPY_DESTINATION,       // ← estava cast(0) inválido
            nri::StageBits::COPY
        };
        barrier.after = {
            nri::AccessBits::SHADER_RESOURCE,
            nri::Layout::SHADER_RESOURCE,
            nri::StageBits::FRAGMENT_SHADER
        };

        nri::BarrierDesc barrierDesc = {};
        barrierDesc.textures = &barrier;
        barrierDesc.textureNum = 1;

        ICore.CmdBarrier(cmdBuffer, barrierDesc);

        bIsDirty = false;
    }

    void Deinit()
    {
        if (Sampler) { ICore.DestroyDescriptor(Sampler);      Sampler = nullptr; }
        if (TextureView) { ICore.DestroyDescriptor(TextureView);  TextureView = nullptr; }
        if (Texture) { ICore.DestroyTexture(Texture);         Texture = nullptr; }
        for (nri::Memory* m : Memory) if (m) ICore.FreeMemory(m);
        Memory.clear();
        TexWidth = TexHeight = 0;
        MipLevels = 1;
        bIsDirty = false;
    }

    bool IsValid() const { return Texture != nullptr; }
    uint32_t GetWidth() const { return (uint32_t)TexWidth; }
    uint32_t GetHeight() const { return (uint32_t)TexHeight; }
    uint32_t GetMipLevels() const { return MipLevels; }
    bool IsDirty()
    {
        return bIsDirty;
    }
    bool NeedsMipmapGeneration() const { return bIsDirty && MipLevels > 1; }
    void ClearDirty() { bIsDirty = false; }

private:
    bool UploadMips(stbi_uc* basePixels)
    {
        int mipW = TexWidth, mipH = TexHeight;
        std::vector<uint8_t> prevMip(basePixels, basePixels + mipW * mipH * 4);

        for (uint32_t mip = 0; mip < MipLevels; mip++)
        {
            std::vector<uint8_t> mipData;
            const void* uploadData = nullptr;

            if (mip == 0)
            {
                uploadData = prevMip.data();
            }
            else
            {
                int newW = std::max(1, mipW / 2);
                int newH = std::max(1, mipH / 2);
                mipData.resize(newW * newH * 4);

                stbir_resize_uint8_linear(
                    prevMip.data(), mipW, mipH, mipW * 4,
                    mipData.data(), newW, newH, newW * 4,
                    STBIR_RGBA);

                mipW = newW;
                mipH = newH;
                prevMip = mipData;
                uploadData = mipData.data();
            }

            uint64_t dataSize = (uint64_t)mipW * mipH * 4;

            nri::DataSize chunk = { uploadData, dataSize };
            nri::StreamBufferDataDesc streamDesc = {};
            streamDesc.dataChunks = &chunk;
            streamDesc.dataChunkNum = 1;
            streamDesc.placementAlignment = 1;

            // dstTexture = nullptr: streaming pra buffer de staging,
            // depois CmdCopyStreamedData faz a cópia pra textura
            nri::TextureRegionDesc region = {};
            region.mipOffset = (nri::Dim_t)mip;
            region.layerOffset = 0;
            region.width = (nri::Dim_t)mipW;
            region.height = (nri::Dim_t)mipH;
            region.depth = 1;

            nri::StreamTextureDataDesc texStreamDesc = {};
            texStreamDesc.data = uploadData;
            texStreamDesc.dataRowPitch = (uint32_t)(mipW * 4);
            texStreamDesc.dataSlicePitch = (uint32_t)(mipW * mipH * 4);
            texStreamDesc.dstTexture = Texture;
            texStreamDesc.dstRegion = region;

            IStreamer.StreamTextureData(*Streamer, texStreamDesc);
        }

        bIsDirty = true;

        return true;
    }

    bool CreateSampler()
    {
        bool useAniso = GUserSettings->Anisotropic != ANISOTROPIC_DISABLED;

        nri::SamplerDesc samplerDesc = {};
        samplerDesc.addressModes = { AddrMode, AddrMode, AddrMode };
        samplerDesc.filters.min = FilterMode;
        samplerDesc.filters.mag = useAniso ? nri::Filter::LINEAR : FilterMode;
        samplerDesc.filters.mip = useAniso ? nri::Filter::LINEAR : FilterMode;
        samplerDesc.anisotropy = (uint8_t)GUserSettings->Anisotropic;
        samplerDesc.mipMin = 0.0f;
        samplerDesc.mipMax = MipLevels > 1 ? (float)(MipLevels - 1) : 0.0f;

        nri::Result result = ICore.CreateSampler(*Device, samplerDesc, Sampler);
        if (result != nri::Result::SUCCESS)
        {
			Logs::Error("NRITexture: failed to create sampler (result: %d)", (int)result);
			return false;
        }

        return true;
    }
};
