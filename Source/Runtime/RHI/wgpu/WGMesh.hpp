#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/RHI/wgpu/WGMaterial.hpp"
#include <dawn/webgpu_cpp.h>


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

    void Draw(wgpu::RenderPassEncoder& pass)
    {
        pass.SetPipeline(Material->Pipeline->RenderPipeline);
        for (auto& texture : Material->Textures)
        {
            pass.SetBindGroup(0, texture->TextureBindGroup);
        }

        pass.SetBindGroup(1, Material->Pipeline->CameraBindGroup);
        pass.SetVertexBuffer(0, VertexBuffer->Buffer.Get(), VertexBuffer->VertexOffset, VertexBuffer->VertexSize);
        pass.SetIndexBuffer(VertexBuffer->Buffer.Get(), wgpu::IndexFormat::Uint32, VertexBuffer->IndexOffset, VertexBuffer->IndexSize);
        //uint32_t indexCount = VertexBuffer->IndexBuffer.GetSize() / sizeof(uint32_t);
        uint32_t indexCount = VertexBuffer->IndexCount;
        pass.DrawIndexed(indexCount, 1, 0, 0, 0);
    }
};
