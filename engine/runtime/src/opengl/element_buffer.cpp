#include "opengl/element_buffer.h"

namespace runa::runtime::opengl {
    ElementBuffer::~ElementBuffer() {
        if (deferDeinit && id > 0) 
            deinit();
    }

    void ElementBuffer::init(const std::vector<uint32_t>& indices)
    {
        glGenBuffers(1, &id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);
        size = indices.size();
    }

    void ElementBuffer::deinit()
    {
        glDeleteBuffers(1, &id);
        id = 0;
        size = 0;
    }

    void ElementBuffer::defer(bool value)
    {
        deferDeinit = value;
    }

    void ElementBuffer::bind() const {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    }

    void ElementBuffer::unbind() const {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    GLsizeiptr ElementBuffer::count() const
    {
        return size;
    }
}
