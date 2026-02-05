#pragma once

#include <vector>
#include <glad/glad.h>

namespace runa::runtime::opengl {
    class ElementBuffer {
    public:
        ElementBuffer() = default;
        ~ElementBuffer();

        void init(const std::vector<uint32_t>& indices);
        void deinit();
        void defer(bool value = true);

        void bind() const;
        void unbind() const;

        GLsizeiptr count() const;
    private:
        bool deferDeinit = true;
        uint32_t id = 0;
        GLsizeiptr size = 0;
    };
}