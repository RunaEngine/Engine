#pragma once

#include <glad/glad.h>

namespace runa::runtime {
    class element_buffer_c {
    public:
        element_buffer_c(const GLuint *indices, const GLsizeiptr size);
        ~element_buffer_c();

        void bind() const;
        void unbind() const;
    private:
        GLuint id;
        GLsizeiptr size;
    };
}