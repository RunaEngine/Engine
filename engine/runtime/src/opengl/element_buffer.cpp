#include "opengl/element_buffer.h"
#include "opengl/element_count.h"

namespace runa::runtime {
    element_buffer_c::element_buffer_c(const GLuint *indices, const GLsizeiptr size) {
        id = 0;
        this->size = 0;
        glGenBuffers(1, &id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
        this->size = size;
        GL_ELEMENT_COUNT += size / sizeof(GLuint);
    }

    element_buffer_c::~element_buffer_c() {
        GL_ELEMENT_COUNT -= size;
        if (GL_ELEMENT_COUNT < 0) GL_ELEMENT_COUNT = 0;
        glDeleteBuffers(1, &id);
    }

    void element_buffer_c::bind() const {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    }

    void element_buffer_c::unbind() const {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
}
