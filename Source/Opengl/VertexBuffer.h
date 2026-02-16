#pragma once

#include "Engine/Core/Object.h"
#include <array>
#include <glad/glad.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec3 Color;
    glm::vec2 TexUV;
};

class GLVertexBuffer: public Object
{
public:
    GLVertexBuffer() = default;
    ~GLVertexBuffer() override;

    void Init(const std::vector<Vertex>& vertices);
    void Deinit();

    void Bind() const;
    void Unbind() const;

private:
    GLuint Id = 0;
};
