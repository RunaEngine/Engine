#pragma once

#include "Engine/Core/Object.hpp"
#include <webgpu/wgpu.h>


class WGBuffer : public Object
{
private:
    WGPUDevice Device = nullptr;
    WGPUQueue Queue = nullptr;
    WGPUBuffer Buffer = nullptr;
    uint64_t Size = 0;
    WGPUBufferUsage Usage = WGPUBufferUsage_None;
    bool Uploaded = false;

public:
    WGBuffer(WGPUDevice device, WGPUQueue queue) : Device(device), Queue(queue)
    {
    }

    ~WGBuffer() override
    {
        Deinit();
    }

    bool Init(uint64_t size, WGPUBufferUsage usage, bool mappedAtCreation = false)
    {
        Deinit();

        Usage = usage;
        Size = size;

        WGPUBufferDescriptor desc = {
            .usage = usage,
            .size = size,
            .mappedAtCreation = mappedAtCreation
        };

        Buffer = wgpuDeviceCreateBuffer(
            Device,
            &desc
        );

        return Buffer != nullptr;
    }

    bool Upload(const void* data, uint64_t size, uint64_t offset = 0)
    {
        if (!Buffer)
            return false;

        if (offset + size > Size)
            return false;

        wgpuQueueWriteBuffer(
            Queue,
            Buffer,
            offset,
            data,
            size
        );

        Uploaded = true;

        return true;
    }

    void Destroy()
    {
        if (Buffer)
            wgpuBufferDestroy(Buffer);

        Uploaded = false;
    }

    void* Map()
    {
        return wgpuBufferGetMappedRange(
            Buffer,
            0,
            Size
        );
    }

    void Unmap()
    {
        wgpuBufferUnmap(Buffer);
    }

    void Deinit()
    {
        if (Buffer)
        {
            Destroy();
            wgpuBufferRelease(Buffer);

            Buffer = nullptr;
        }

        Size = 0;
        Usage = WGPUBufferUsage_None;
    }

    WGPUBuffer Get() const
    {
        return Buffer;
    }

    uint64_t GetSize() const
    {
        return Size;
    }

    WGPUBufferUsage GetUsage() const
    {
        return Usage;
    }

    bool IsValid() const
    {
        return Buffer != nullptr;
    }

    bool IsUploaded() const
    {
        return Uploaded;
    }
};
