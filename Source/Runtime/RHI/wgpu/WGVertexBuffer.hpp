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
    WGBuffer VertexBuffer;
    WGBuffer IndexBuffer;

    WGVertexBuffer(wgpu::Device device, wgpu::Queue queue) : VertexBuffer(device, queue), IndexBuffer(device, queue)
    {
    }

    ~WGVertexBuffer() override
    {
        Deinit();
    }

    void Init(const std::vector<WGVertex>& vertices, const std::vector<uint32_t>& indices)
    {
        VertexBuffer.Init(sizeof(WGVertex) * vertices.size(), wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst);
        IndexBuffer.Init(sizeof(uint32_t) * indices.size(), wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst);
        if (!vertices.empty())
        {
            VertexBuffer.Upload(vertices.data(), sizeof(WGVertex) * vertices.size(), 0);
        }
        if (!indices.empty())
        {
            IndexBuffer.Upload(indices.data(), sizeof(uint32_t) * indices.size(), 0);
        }
    }

    void Deinit()
    {
        VertexBuffer.Deinit();
        IndexBuffer.Deinit();
    }
};
