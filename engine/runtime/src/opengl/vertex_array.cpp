#include "opengl/vertex_array.h"

namespace runa::runtime {
    vertex_array_c::vertex_array_c() {
        glGenVertexArrays(1, &id);
    }

    vertex_array_c::~vertex_array_c() {
        glDeleteVertexArrays(1, &id);
    }

    void vertex_array_c::bind() const {
        glBindVertexArray(id);
    }

    void vertex_array_c::unbind() const {
        glBindVertexArray(0);
    }

    void vertex_array_c::enable_attrib(const vertex_buffer_c &vertex_buffer, const GLuint layout, GLuint num, GLenum type, GLsizeiptr stride, void *offset) const {
        vertex_buffer.bind();
        glVertexAttribPointer(layout, num, type, GL_FALSE, stride, offset);
        glEnableVertexAttribArray(layout);
        vertex_buffer.unbind();
    }
}
