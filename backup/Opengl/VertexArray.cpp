#include "Opengl/VertexArray.h"

GLVertexArray::~GLVertexArray()
{
    Deinit();
}

void GLVertexArray::Init()
{
    glGenVertexArrays(1, &Id);
}

void GLVertexArray::Deinit()
{
    glDeleteVertexArrays(1, &Id);
}

void GLVertexArray::Bind() const
{
    glBindVertexArray(Id);
}

void GLVertexArray::Unbind() const
{
    glBindVertexArray(0);
}

void GLVertexArray::EnableAttrib(const GLVertexBuffer& vertexBuffer, const GLuint layout, GLuint num, GLenum type,
                               GLsizeiptr stride, void* offset) const
{
    vertexBuffer.Bind();
    glVertexAttribPointer(layout, num, type, GL_FALSE, stride, offset);
    glEnableVertexAttribArray(layout);
    vertexBuffer.Unbind();
}
