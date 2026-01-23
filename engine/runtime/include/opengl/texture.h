#pragma once

#include "shader.h"
#include <glad/glad.h>

namespace runa::runtime::opengl {
    class Texture {
    public:
        Texture() = default;
        ~Texture();

        bool init(const char* texturefile, const char* textype, GLenum slot, GLenum channels, GLenum pixeltype);
        void denit();

        void texUnit(const Shader& shader, const char* uniform, GLuint unit);

        void bind() const;
        void unbind() const;

        const char* getType();
    private:
        GLuint id = 0;
        const char* type = 0;
        GLuint unit = 0;
    };
}
