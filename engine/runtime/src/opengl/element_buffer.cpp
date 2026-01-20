#include "opengl/element_buffer.h"

namespace runa::runtime::opengl {
    ElementBuffer::~ElementBuffer() {
        if (id > 0) deinit();
    }

    void ElementBuffer::init(const GLuint* indices, GLsizeiptr count)
    {
        glGenBuffers(1, &id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(GLuint), indices, GL_STATIC_DRAW);
        size = count;
    }

    void ElementBuffer::deinit()
    {
        glDeleteBuffers(1, &id);
        id = 0;
        size = 0;
    }

    void ElementBuffer::bind() const {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    }

    void ElementBuffer::unbind() const {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    GLsizeiptr ElementBuffer::count() const
    {
        return size / sizeof(GLuint);
    }
}
