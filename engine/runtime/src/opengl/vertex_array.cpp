#include "opengl/vertex_array.h"

namespace runa::runtime::opengl {
    VertexArray::~VertexArray() {
        if (id > 0) deinit();
    }

    void VertexArray::init()
    {
        glGenVertexArrays(1, &id);
    }

    void VertexArray::deinit()
    {
        glDeleteVertexArrays(1, &id);
    }

    void VertexArray::bind() const {
        glBindVertexArray(id);
    }

    void VertexArray::unbind() const {
        glBindVertexArray(0);
    }

    void VertexArray::enableAttrib(const VertexBuffer &vertex_buffer, const GLuint layout, GLuint num, GLenum type, GLsizeiptr stride, void *offset) const {
        vertex_buffer.bind();
        glVertexAttribPointer(layout, num, type, GL_FALSE, stride, offset);
        glEnableVertexAttribArray(layout);
        vertex_buffer.unbind();
    }
}
