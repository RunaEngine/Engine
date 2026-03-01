#include "Opengl/ElementBuffer.h"

GLElementBuffer::~GLElementBuffer()
{
    Deinit();
}

void GLElementBuffer::Init(const std::vector<uint32_t>& indices)
{
    glGenBuffers(1, &Id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);
    Size = indices.size();
}

void GLElementBuffer::Deinit()
{
    glDeleteBuffers(1, &Id);
    Id = 0;
    Size = 0;
}

void GLElementBuffer::Bind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Id);
}

void GLElementBuffer::Unbind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

GLsizeiptr GLElementBuffer::Count() const
{
    return Size;
}
