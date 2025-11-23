#pragma once

#include <glad/glad.h>

namespace runa::runtime {
    class vertex_buffer_c {
    public:
        vertex_buffer_c(const GLfloat *vertices, const GLsizeiptr size);
        ~vertex_buffer_c();

        void bind() const;
        void unbind() const;
    private:
        GLuint id;
    };
}