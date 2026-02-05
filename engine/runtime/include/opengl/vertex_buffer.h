#pragma once

#include <array>
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

        void init(const std::vector<Vertex>& vertices);
        void deinit();
        void defer(bool value = true);

        void bind() const;
        void unbind() const;
    private:
        bool deferDeinit = true;
        GLuint id;
    };
}
