#pragma once

#include "Engine/Core/Object.hpp"
#include <NRI.h>
#include <Extensions/NRIStreamer.h>

#include "Runtime/Utils/Logs.hpp"

class NRIStreamer : Object
{
private:
    nri::StreamerInterface& IStreamer;
    nri::Device* Device = nullptr;
    nri::Streamer* Streamer = nullptr;
    uint64_t Size = 0;

public:
    NRIStreamer(nri::StreamerInterface& istreamer, nri::Device* device) : Device(device), IStreamer(istreamer)
    {
    }
    virtual ~NRIStreamer() override
    {
        Deinit();
    }

    nri::Result Init(uint64_t size, nri::BufferUsageBits usage)
    {
        Size = size;
        nri::StreamerDesc streamerDesc = {};
        streamerDesc.dynamicBufferMemoryLocation = nri::MemoryLocation::HOST_UPLOAD;
        streamerDesc.dynamicBufferDesc = {
            .size = size,
            .structureStride = 0,
            .usage = usage,
        };
        streamerDesc.constantBufferMemoryLocation = nri::MemoryLocation::HOST_UPLOAD;

        auto result = IStreamer.CreateStreamer(*Device, streamerDesc, Streamer);

        if (result != nri::Result::SUCCESS)
        {
            Logs::Error("NRIStreamer: CreateStreamer failed: %d", (int)result);
            Deinit();
        }

        return result;
    }

    void Deinit()
    {
        if (Streamer)
        {
            IStreamer.DestroyStreamer(Streamer);
            Streamer = nullptr;
            Size = 0;
        }
    }

    bool Upload(const void* data, uint64_t size, uint64_t offset = 0)
    {
        if (!Streamer)
            return false;

        if (offset + size > Size)
            return false;


        return true;
    }

    nri::Streamer* Get() const
    {
        return Streamer;
    }

    uint64_t GetSize() const
    {
        return Size;
    }
};
