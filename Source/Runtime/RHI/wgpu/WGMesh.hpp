#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/RHI/wgpu/WGMaterial.hpp"
#include <webgpu/wgpu.h>


class WGMesh : Object
{
public:
    SharedPtr<WGMaterial> Material = nullptr;
    SharedPtr<WGVertexBuffer> VertexBuffer = nullptr;

    WGMesh(SharedPtr<WGVertexBuffer> vertexBuffer, SharedPtr<WGMaterial> material) : VertexBuffer(vertexBuffer),
        Material(material)
    {
    }

    ~WGMesh() override = default;

    void Draw(WGPURenderPassEncoder pass)
    {
        wgpuRenderPassEncoderSetPipeline(pass, Material->Pipeline->RenderPipeline);
        wgpuRenderPassEncoderSetBindGroup(
            pass,
            0,
            Material->Textures[0]->TextureBindGroup,
            0,
            nullptr
        );
        wgpuRenderPassEncoderSetBindGroup(
            pass,
            1,
            Material->Pipeline->Camera->CameraBindGroup,
            0,
            nullptr
        );
        wgpuRenderPassEncoderSetVertexBuffer(
            pass,
            0,
            VertexBuffer->VertexBuffer.Get(),
            0,
            VertexBuffer->VertexBuffer.GetSize()
        );
        wgpuRenderPassEncoderSetIndexBuffer(
            pass,
            VertexBuffer->IndexBuffer.Get(),
            WGPUIndexFormat_Uint32,
            0,
            VertexBuffer->IndexBuffer.GetSize()
        );
        uint32_t indexCount = VertexBuffer->IndexBuffer.GetSize() / sizeof(uint32_t);
        wgpuRenderPassEncoderDrawIndexed(
            pass,
            indexCount,
            1,
            0,
            0,
            0
        );
    }
};
