#pragma once

#include "Opengl/VertexBuffer.h"
#include "Engine/Core/Object.h"
#include <glad/glad.h>

class VertexArray : public Object
{
public:
    VertexArray() = default;
    ~VertexArray() override;

    void Init();
    void Deinit();

    void Bind() const;
    void Unbind() const;
    void EnableAttrib(const VertexBuffer& vertexBuffer, GLuint layout, GLuint num, GLenum type, GLsizeiptr stride,
                      void* offset) const;

private:
    GLuint Id = 0;
};
