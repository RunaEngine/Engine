#pragma once

#include "shader.h"
#include <glad/glad.h>
#include <filesystem>

namespace runa::runtime::opengl {
    class Texture {
    public:
        Texture() = default;
        ~Texture();

        bool init(const std::filesystem::path& texturefile, const char* textype, GLenum slot, GLenum channels, GLenum pixeltype);
        bool init(const std::vector<uint8_t>& buf, const char* textype, GLenum slot, GLenum channels, GLenum pixeltype);
        void denit();
        void defer(bool value = true);

        void texUnit(const Shader& shader, const char* uniform, GLuint unit);

        void bind() const;
        void unbind() const;

        const char* getType();
        GLuint getID();
    private:
        bool deferDeinit = true;
        GLuint id = 0;
        const char* type = 0;
        GLuint unit = 0;
    };
}
