#include "opengl/vertex_buffer.h"

namespace runa::runtime::opengl {
    VertexBuffer::~VertexBuffer()
    {
        if (deferDeinit && id > 0) 
            deinit();
    }

    void VertexBuffer::init(const std::vector<Vertex>& vertices)
    {
        glGenBuffers(1, &id);
        glBindBuffer(GL_ARRAY_BUFFER, id);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    }

    void VertexBuffer::deinit()
    {
        glDeleteBuffers(1, &id);
        id = 0;
    }

    void VertexBuffer::defer(bool value)
    {
        deferDeinit = value;
    }

    void VertexBuffer::bind() const {
        glBindBuffer(GL_ARRAY_BUFFER, id);
    }

    void VertexBuffer::unbind() const {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}