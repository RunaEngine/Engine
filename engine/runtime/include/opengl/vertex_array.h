#pragma once

#include "opengl/vertex_buffer.h"
#include <glad/glad.h>

namespace runa::runtime {
    class vertex_array_c {
    public:
        vertex_array_c();
        ~vertex_array_c();

        void bind() const;
        void unbind() const;
        void enable_attrib(const vertex_buffer_c &vertex_buffer, const GLuint layout, GLuint num, GLenum type, GLsizeiptr stride, void *offset) const;
    private:
        GLuint id;
    };
}