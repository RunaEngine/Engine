#pragma once
#include "Engine/Core/Object.hpp"
#include "Runtime/RHI/NRI/NRIMaterial.hpp"
#include "Runtime/RHI/NRI/NRIVertexBuffer.hpp"
#include <NRI.h>

class NRIMesh : public Object
{
public:
    SharedPtr<NRIMaterial> Material = nullptr;
    SharedPtr<NRIVertexBuffer> VertexBuffer = nullptr;

    NRIMesh(SharedPtr<NRIVertexBuffer> vertexBuffer, SharedPtr<NRIMaterial> material)
        : VertexBuffer(vertexBuffer), Material(material)
    {}

    ~NRIMesh() override = default;

    void Draw(nri::CoreInterface& core, nri::CommandBuffer& commandBuffer)
    {
        if (!Material || !VertexBuffer || !Material->Pipeline || !Material->Pipeline->Pipeline) return;

        // Bind the pipeline layout, pipeline state, and descriptor sets (textures, camera)
        Material->Bind(commandBuffer);

        nri::Buffer* buffer = VertexBuffer->Buffer.Get();
        if (!buffer) return;

        // Set Vertex Buffer
        nri::VertexBufferDesc vbDesc = {};
        vbDesc.buffer = buffer;
        vbDesc.offset = VertexBuffer->VertexOffset;
        vbDesc.stride = sizeof(NRIVertex);
        core.CmdSetVertexBuffers(commandBuffer, 0, &vbDesc, 1);

        // Set Index Buffer
        core.CmdSetIndexBuffer(commandBuffer, *buffer, VertexBuffer->IndexOffset, nri::IndexType::UINT32);

        // Draw Geometry
        // Fixed: Adjusted to match the exact signature: void(* CmdDrawIndexed)(CommandBuffer&, const DrawIndexedDesc&)
        nri::DrawIndexedDesc drawDesc = {};
        drawDesc.indexNum = VertexBuffer->IndexCount;
        drawDesc.instanceNum = 1;
        drawDesc.baseIndex = 0;
        drawDesc.baseVertex = 0;
        drawDesc.baseInstance = 0;

        core.CmdDrawIndexed(commandBuffer, drawDesc);
    }
};