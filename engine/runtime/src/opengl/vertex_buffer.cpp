#include "opengl/vertex_buffer.h"

namespace runa::runtime::opengl {
    VertexBuffer::~VertexBuffer()
    {
        if (id > 0) deinit();
    }

    void VertexBuffer::init(const Vertex* vertices, GLsizeiptr count)
    {
        glGenBuffers(1, &id);
        glBindBuffer(GL_ARRAY_BUFFER, id);
        glBufferData(GL_ARRAY_BUFFER, count * sizeof(Vertex), vertices, GL_STATIC_DRAW);
    }

    void VertexBuffer::deinit()
    {
        glDeleteBuffers(1, &id);
        id = 0;
    }

    void VertexBuffer::bind() const {
        glBindBuffer(GL_ARRAY_BUFFER, id);
    }

    void VertexBuffer::unbind() const {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}