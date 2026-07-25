#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/RHI/wgpu/WGBuffer.hpp"
#include <glm/glm.hpp>
#include <webgpu/wgpu.h>
#include <array>

struct WGVertex
{
    glm::vec3 Position;
    glm::vec2 TexCoord;
    //float Color[3];

    static WGPUVertexBufferLayout GetLayout()
    {
        static std::array<WGPUVertexAttribute, 2> attributes = {};

        attributes[0].format = WGPUVertexFormat_Float32x3;
        attributes[0].offset = offsetof(WGVertex, Position);
        attributes[0].shaderLocation = 0;

        // attributes[1].format = WGPUVertexFormat_Float32x3;
        // attributes[1].offset = offsetof(WGVertex, Color);
        // attributes[1].shaderLocation = 1;

        attributes[1].format = WGPUVertexFormat_Float32x2;
        attributes[1].offset = offsetof(WGVertex, TexCoord);
        attributes[1].shaderLocation = 1;

        WGPUVertexBufferLayout layout = {
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

    WGVertexBuffer(WGPUDevice device, WGPUQueue queue) : VertexBuffer(device, queue), IndexBuffer(device, queue)
    {
    }

    ~WGVertexBuffer() override
    {
        Deinit();
    }

    void Init(const std::vector<WGVertex>& vertices, const std::vector<uint32_t>& indices)
    {
        VertexBuffer.Init(sizeof(WGVertex) * vertices.size(), WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst);
        IndexBuffer.Init(sizeof(uint32_t) * indices.size(), WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst);
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
