#include "Opengl/VertexBuffer.h"

GLVertexBuffer::~GLVertexBuffer()
{
    Deinit();
}

void GLVertexBuffer::Init(const std::vector<Vertex>& vertices)
{
    glGenBuffers(1, &Id);
    glBindBuffer(GL_ARRAY_BUFFER, Id);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
}

void GLVertexBuffer::Deinit()
{
    glDeleteBuffers(1, &Id);
    Id = 0;
}

void GLVertexBuffer::Bind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, Id);
}

void GLVertexBuffer::Unbind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
