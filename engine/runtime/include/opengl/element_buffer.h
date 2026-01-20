#pragma once

#include <glad/glad.h>

namespace runa::runtime::opengl {
    class ElementBuffer {
    public:
        ElementBuffer() = default;
        ~ElementBuffer();

        void init(const GLuint *indices, GLsizeiptr count);
        void deinit();

        void bind() const;
        void unbind() const;

        GLsizeiptr count() const;
    private:
        GLuint id = 0;
        GLsizeiptr size = 0;
    };
}