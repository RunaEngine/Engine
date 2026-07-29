#pragma once

#include "Engine/Core/Object.hpp"
#include <dawn/webgpu_cpp.h>


class WGBuffer : public Object
{
private:
    wgpu::Device Device;
    wgpu::Queue Queue;

    wgpu::Buffer Buffer = nullptr;
    uint64_t Size = 0;
    wgpu::BufferUsage Usage = wgpu::BufferUsage::None;

public:
    WGBuffer(wgpu::Device device, wgpu::Queue queue) : Device(device), Queue(queue)
    {
    }

    ~WGBuffer() override
    {
        Deinit();
    }

    bool Init(uint64_t size, wgpu::BufferUsage usage, bool mappedAtCreation = false)
    {
        Deinit();

        Usage = usage;
        Size = size;

        wgpu::BufferDescriptor desc = {
            .usage = usage,
            .size = size,
            .mappedAtCreation = mappedAtCreation
        };

        Buffer = Device.CreateBuffer(&desc);

        return Buffer != nullptr;
    }

    bool Upload(const void* data, uint64_t size, uint64_t offset = 0)
    {
        if (!Buffer)
            return false;

        if (offset + size > Size)
            return false;


        Queue.WriteBuffer(Buffer, offset, data, size);

        return true;
    }

    void Deinit()
    {
        Buffer = nullptr;

        Size = 0;
        Usage = wgpu::BufferUsage::None;
    }

    wgpu::Buffer Get() const
    {
        return Buffer;
    }

    uint64_t GetSize() const
    {
        return Size;
    }

    wgpu::BufferUsage GetUsage() const
    {
        return Usage;
    }

    bool IsValid() const
    {
        return Buffer != nullptr;
    }
};

/*
    void Destroy()
    {
        if (Buffer)
            Buffer.Destroy();

        Uploaded = false;
    }

    const void* Map()
    {

        return Buffer.GetConstMappedRange(0, Size);
    }

    void Unmap()
    {
        Buffer.Unmap();
    }
*/