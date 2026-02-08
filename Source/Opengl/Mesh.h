#pragma once

#include "Opengl/VertexArray.h"
#include "Opengl/ElementBuffer.h"
#include "Opengl/Camera.h"
#include "Opengl/Texture.h"
#include "Engine/Core/Object.h"

class Mesh : public Object
{
public:
    Mesh() = default;
    ~Mesh() override;

    bool Init(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices, const std::vector<SharedPtr<Texture>>& textures);
    bool Init(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices);
    void Deinit();

    void Draw(const Shader& shader, const Camera& camera,
              glm::mat4 matrix = glm::mat4(1.0f),
              glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
              glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
              glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f)
    );

private:
    std::vector<SharedPtr<Texture>> Textures;
    // Store VAO in public so it can be used in the Draw function
    SharedPtr<VertexArray> Vao = MakeShared<VertexArray>();
    SharedPtr<VertexBuffer> Vbo = MakeShared<VertexBuffer>();
    SharedPtr<ElementBuffer> Ebo = MakeShared<ElementBuffer>();
};
