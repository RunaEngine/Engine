#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/RHI/wgpu/WGBuffer.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <webgpu/wgpu.h>
#include <array>

struct InstanceRaw
{
    glm::mat4 Model;

    static WGPUVertexBufferLayout GetLayout()
    {
        static std::array<WGPUVertexAttribute, 3> attributes = {};

        attributes[0].format = WGPUVertexFormat_Float32x4;
        attributes[0].offset = 0;
        attributes[0].shaderLocation = 5;

        attributes[1].format = WGPUVertexFormat_Float32x4;
        attributes[1].offset = sizeof(float) * 4;
        attributes[1].shaderLocation = 6;

        attributes[2].format = WGPUVertexFormat_Float32x4;
        attributes[2].offset = sizeof(float) * 8;
        attributes[2].shaderLocation = 7;

        attributes[3].format = WGPUVertexFormat_Float32x4;
        attributes[3].offset = sizeof(float) * 12;
        attributes[3].shaderLocation = 8;

        WGPUVertexBufferLayout layout = {
            .stepMode = WGPUVertexStepMode_Instance,
            .arrayStride = sizeof(InstanceRaw),
            .attributeCount = static_cast<uint32_t>(attributes.size()),
            .attributes = attributes.data(),
        };

        return layout;
    }
};

struct WGInstance
{
    glm::vec3 Position;
    glm::quat Rotation;

    InstanceRaw ToInstance() const
    {
        glm::mat4 identity = glm::mat4(1.0f);
        glm::mat4 translationMat = glm::translate(identity, Position);
        glm::mat4 rotationMat = glm::toMat4(Rotation);

        return InstanceRaw{
            .Model = translationMat * rotationMat
        };
    }
};

class WGInstanceBuffer : Object
{
public:
    std::vector<WGInstance> Instances;
    WGBuffer Buffer;

    WGInstanceBuffer();
    ~WGInstanceBuffer();
};
