#pragma once

#include <glad/glad.h>

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

namespace runa::runtime::opengl {
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 color;
        glm::vec2 texUV;
    };

    class VertexBuffer {
    public:
        VertexBuffer() = default;
        ~VertexBuffer();

        void init(const Vertex* vertices, GLsizeiptr count);
        void deinit();

        void bind() const;
        void unbind() const;
    private:
        GLuint id;
    };
}
