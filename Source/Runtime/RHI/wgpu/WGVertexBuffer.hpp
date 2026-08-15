#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/RHI/wgpu/WGBuffer.hpp"
#include <glm/glm.hpp>
#include <dawn/webgpu_cpp.h>
#include <array>

struct WGVertex
{
    glm::vec3 Position;
    glm::vec2 TexCoord;
    //float Color[3];

    static wgpu::VertexBufferLayout GetLayout()
    {
        static std::array<wgpu::VertexAttribute, 2> attributes = {};

        attributes[0].format = wgpu::VertexFormat::Float32x3;
        attributes[0].offset = offsetof(WGVertex, Position);
        attributes[0].shaderLocation = 0;

        // attributes[1].format = wgpu::VertexFormat_Float32x3;
        // attributes[1].offset = offsetof(WGVertex, Color);
        // attributes[1].shaderLocation = 1;

        attributes[1].format = wgpu::VertexFormat::Float32x2;
        attributes[1].offset = offsetof(WGVertex, TexCoord);
        attributes[1].shaderLocation = 1;

        wgpu::VertexBufferLayout layout = {
            .arrayStride = sizeof(WGVertex),
            .attributeCount = static_cast<uint32_t>(attributes.size()),
            .attributes = attributes.data()
        };

        return layout;
    }
};

class WGVertexBuffer : Object
{
public:
    WGBuffer Buffer;

    uint64_t VertexOffset = 0;
    uint64_t VertexSize = 0;
    uint64_t IndexOffset = 0;
    uint64_t IndexSize = 0;
    uint32_t IndexCount = 0;

    WGVertexBuffer(wgpu::Device device, wgpu::Queue queue) : Buffer(device, queue)
    {
    }

    ~WGVertexBuffer() override
    {
        Deinit();
    }

    void Init(const std::vector<WGVertex>& vertices, const std::vector<uint32_t>& indices)
    {
        if (vertices.empty() || indices.empty())
        {
            return;
        }
        VertexSize = sizeof(WGVertex) * vertices.size();
        IndexSize = sizeof(uint32_t) * indices.size();
        IndexCount = static_cast<uint32_t>(indices.size());

        VertexOffset = 0;
        IndexOffset = VertexSize;

        uint64_t bufferSize = VertexSize + IndexSize;
        wgpu::BufferUsage usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst;

        Buffer.Init(bufferSize, usage);
        Buffer.Upload(vertices.data(), VertexSize, VertexOffset);
        Buffer.Upload(indices.data(), IndexSize, IndexOffset);
    }

    void Deinit()
    {
		Buffer.Deinit();
        //VertexBuffer.Deinit();
        //IndexBuffer.Deinit();
    }
};
