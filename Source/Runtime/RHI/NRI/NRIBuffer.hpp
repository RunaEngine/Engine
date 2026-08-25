#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/Utils/Logs.hpp"

#include <NRI.h>

#include <algorithm>
#include <cstdint>
#include <cstring>

class NRIBuffer : public Object
{
private:
    nri::CoreInterface& ICore;
    nri::Device* Device = nullptr;

    nri::Buffer* Buffer = nullptr;
    nri::Memory* Memory = nullptr;

    uint64_t Capacity = 0;
    uint64_t Size = 0;

public:
    NRIBuffer(nri::CoreInterface& core, nri::Device* device)
        : ICore(core)
        , Device(device)
    {
    }

    ~NRIBuffer()
    {
        Deinit();
    }

    bool Init(
        uint64_t size,
        nri::BufferUsageBits usage,
        nri::MemoryLocation location = nri::MemoryLocation::DEVICE
    )
    {
        Deinit();

        if (size == 0)
        {
            Logs::Error("NRIBuffer: Size cannot be zero");
            return false;
        }

        nri::BufferDesc desc = {};
        desc.size = size;
        desc.usage = usage;

        nri::Result result = ICore.CreateBuffer(
            *Device,
            desc,
            Buffer
        );

        if (result != nri::Result::SUCCESS)
        {
            Logs::Error(
                "NRIBuffer: CreateBuffer failed: %d",
                (int)result
            );

            return false;
        }

        if (!Malloc(location))
        {
            Deinit();
            return false;
        }

        Capacity = size;
        Size = 0;

        return true;
    }

    void Deinit()
    {
        if (Buffer)
        {
            ICore.DestroyBuffer(Buffer);
            Buffer = nullptr;
        }

        if (Memory)
        {
            Free();
        }

        Capacity = 0;
        Size = 0;
    }

    bool Upload(
        const void* data,
        uint64_t size,
        uint64_t offset = 0
    )
    {
        if (!data)
            return false;

        if (!Buffer)
            return false;

        if (size == 0)
            return true;

        if (offset + size > Capacity)
        {
            Logs::Error(
                "NRIBuffer: Upload exceeds buffer size"
            );

            return false;
        }

        void* dstMemory = ICore.MapBuffer(
            *Buffer,
            offset,
            size
        );

        if (!dstMemory)
        {
            Logs::Error(
                "NRIBuffer: MapBuffer failed"
            );

            return false;
        }

        std::memcpy(
            dstMemory,
            data,
            size
        );

        ICore.UnmapBuffer(*Buffer);

        Size = std::max(
            Size,
            offset + size
        );

        return true;
    }

    void* Map(uint64_t offset, uint64_t size)
    {
        return ICore.MapBuffer(*Buffer, offset, size);
    }

    void Unmap()
    {
        ICore.UnmapBuffer(*Buffer);
    }

    nri::Buffer* Get() const
    {
        return Buffer;
    }

    nri::Memory* GetMemory() const
    {
        return Memory;
    }

    uint64_t GetSize() const
    {
        return Size;
    }

    uint64_t GetCapacity() const
    {
        return Capacity;
    }

private:
    bool Malloc(nri::MemoryLocation location)
    {
        nri::MemoryDesc memoryDesc = {};

        ICore.GetBufferMemoryDesc(
            *Buffer,
            location,
            memoryDesc
        );

        nri::AllocateMemoryDesc allocationDesc = {};
        allocationDesc.type = memoryDesc.type;
        allocationDesc.size = memoryDesc.size;

        nri::Result result =
            ICore.AllocateMemory(
                *Device,
                allocationDesc,
                Memory
            );

        if (result != nri::Result::SUCCESS)
        {
            Logs::Error(
                "NRIBuffer: AllocateMemory failed: %d",
                (int)result
            );

            return false;
        }

        nri::BindBufferMemoryDesc bindDesc = {};
        bindDesc.buffer = Buffer;
        bindDesc.memory = Memory;
        bindDesc.offset = 0;

        result =
            ICore.BindBufferMemory(
                &bindDesc,
                1
            );

        if (result != nri::Result::SUCCESS)
        {
            Logs::Error(
                "NRIBuffer: BindBufferMemory failed: %d",
                (int)result
            );

            ICore.FreeMemory(Memory);
            Memory = nullptr;

            return false;
        }

        return true;
    }

    void Free()
    {
        if (Memory)
        {
            ICore.FreeMemory(Memory);
            Memory = nullptr;
        }
    }
};
