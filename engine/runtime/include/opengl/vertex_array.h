#pragma once

#include "opengl/vertex_buffer.h"
#include <glad/glad.h>

namespace runa::runtime::opengl {
    class VertexArray {
    public:
        VertexArray() = default;
        ~VertexArray();

        void init();
        void deinit();

        void bind() const;
        void unbind() const;
        void enableAttrib(const VertexBuffer &vertex_buffer, const GLuint layout, GLuint num, GLenum type, GLsizeiptr stride, void *offset) const;
    private:
        GLuint id;
    };
}