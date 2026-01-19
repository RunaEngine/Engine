#pragma once

#include "shader.h"
#include <glad/glad.h>

namespace runa::runtime {
    class texture_c {
    public:
        texture_c(const std::string &texturefile, const GLenum textype, const GLenum slot,
                   const GLenum format, GLenum pixeltype);
        ~texture_c();

        void bind() const;
        void unbind() const;
        bool is_valid() const { return is_loaded; }
    private:
        GLuint id;
        GLenum type;
        bool is_loaded = false;
    };
}