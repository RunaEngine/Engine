#include "Opengl/VertexArray.h"

VertexArray::~VertexArray()
{
    Deinit();
}

void VertexArray::Init()
{
    glGenVertexArrays(1, &Id);
}

void VertexArray::Deinit()
{
    glDeleteVertexArrays(1, &Id);
}

void VertexArray::Bind() const
{
    glBindVertexArray(Id);
}

void VertexArray::Unbind() const
{
    glBindVertexArray(0);
}

void VertexArray::EnableAttrib(const VertexBuffer& vertexBuffer, const GLuint layout, GLuint num, GLenum type,
                               GLsizeiptr stride, void* offset) const
{
    vertexBuffer.Bind();
    glVertexAttribPointer(layout, num, type, GL_FALSE, stride, offset);
    glEnableVertexAttribArray(layout);
    vertexBuffer.Unbind();
}
