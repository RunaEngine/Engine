#pragma once

#include "Engine/Core/Object.hpp"
#include "Vulkan/VertexBuffer.hpp"
#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <vector>

class Primitive : public Object
{
public:
    SharedPtr<VKVertexBuffer> VertexBuffer;
    glm::mat4 Matrix = glm::mat4(1.0f);
    glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::quat Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 Scale = glm::vec3(1.0f, 1.0f, 1.0f);

    Primitive() = default;
    ~Primitive() override
    {
        Deinit();
    }

    template<typename T>
    void Init(const std::vector<VKVertex>& vertices, const std::vector<T>& indices, const std::vector<SharedPtr<VKTexture>>& textures = {})
    {
        VertexBuffer->Init(vertices, indices, textures);
    }

    void Deinit()
    {
        VertexBuffer->Deinit();
    }

    void Draw()
    {
        VertexBuffer->UpdateUniformBuffer(0);
    }
};