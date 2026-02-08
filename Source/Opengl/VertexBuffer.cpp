#include "Opengl/VertexBuffer.h"

VertexBuffer::~VertexBuffer()
{
    Deinit();
}

void VertexBuffer::Init(const std::vector<Vertex>& vertices)
{
    glGenBuffers(1, &Id);
    glBindBuffer(GL_ARRAY_BUFFER, Id);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
}

void VertexBuffer::Deinit()
{
    glDeleteBuffers(1, &Id);
    Id = 0;
}

void VertexBuffer::Bind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, Id);
}

void VertexBuffer::Unbind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
