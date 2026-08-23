#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/RHI/NRI/NRIBuffer.hpp"
#include "Runtime/Utils/Logs.hpp"
#include <NRI.h>
#include <glm/glm.hpp>
#include <array>
#include <vector>


struct NRIVertex
{
    glm::vec3 Position;
    glm::vec2 TexCoord;

    static const nri::VertexAttributeDesc* GetAttributes(uint32_t& outCount)
    {
        static const std::array<nri::VertexAttributeDesc, 2> attributes =
        {
            nri::VertexAttributeDesc
            {
                .d3d = {"POSITION", 0},
                .vk = {0},
                .offset = offsetof(NRIVertex, Position),
                .format = nri::Format::RGB32_SFLOAT,
                .streamIndex = 0
            },

            nri::VertexAttributeDesc
            {
                .d3d = {"TEXCOORD", 0},
                .vk = {1},
                .offset = offsetof(NRIVertex, TexCoord),
                .format = nri::Format::RG32_SFLOAT,
                .streamIndex = 0
            }
        };

        outCount = static_cast<uint32_t>(attributes.size());

        return attributes.data();
    }

    static nri::VertexStreamDesc GetStreamDesc(
        uint16_t bindingSlot = 0
    )
    {
        nri::VertexStreamDesc streamDesc = {};

        streamDesc.bindingSlot = bindingSlot;
        streamDesc.stepRate = nri::VertexStreamStepRate::PER_VERTEX;
        streamDesc.stride = sizeof(NRIVertex);

        return streamDesc;
    }
};


class NRIVertexBuffer : public Object
{
private:
    nri::CoreInterface& ICore;
    nri::Device* Device = nullptr;

public:
    NRIBuffer Buffer;

    uint64_t VertexOffset = 0;
    uint64_t VertexSize = 0;
    uint64_t IndexOffset = 0;
    uint64_t IndexSize = 0;
    uint32_t IndexCount = 0;

    NRIVertexBuffer(nri::CoreInterface& core, nri::Device* device)
        : ICore(core), Device(device), Buffer(core, device)
    {
    }

    ~NRIVertexBuffer() override
    {
        Deinit();
    }

    bool Init(const std::vector<NRIVertex>& vertices, const std::vector<uint32_t>& indices)
    {
        Deinit();

        if (vertices.empty())
        {
            Logs::Error("NRIVertexBuffer: Vertex data is empty");
            return false;
        }

        if (indices.empty())
        {
            Logs::Error("NRIVertexBuffer: Index data is empty");
            return false;
        }

        VertexSize = sizeof(NRIVertex) * vertices.size();
        IndexSize = sizeof(uint32_t) * indices.size();
        IndexCount = static_cast<uint32_t>(indices.size());

        VertexOffset = 0;
        IndexOffset = VertexSize;

        uint64_t bufferSize = VertexSize + IndexSize;
        nri::BufferUsageBits usage = nri::BufferUsageBits::VERTEX_BUFFER | nri::BufferUsageBits::INDEX_BUFFER;

        if (!Buffer.Init(bufferSize, usage, nri::MemoryLocation::HOST_UPLOAD))
        {
            Deinit();
            return false;
        }
        if (!Buffer.Upload(vertices.data(), VertexSize, VertexOffset))
        {
            Deinit();
            return false;
        }
        if (!Buffer.Upload(indices.data(), IndexSize, IndexOffset))
        {
            Deinit();
            return false;
        }

        return true;
    }

    void Deinit()
    {
        VertexSize = 0;
        IndexSize = 0;
        IndexCount = 0;

        VertexOffset = 0;
        IndexOffset = 0;

        Buffer.Deinit();
    }
};
