#pragma once

#include <string>
#include <glad/glad.h>
#include <filesystem>

namespace runa::runtime::opengl {
    class Shader {
    public:
        Shader() = default;
        ~Shader();

        bool init(const std::filesystem::path& vertexfile, const std::filesystem::path& fragmentfile);
        void deinit();
        void defer(bool value = true);

        void use() const;
        void setUniformLocation(const char *uniform, GLuint unit) const;
        GLuint getID() const { return id; }
    private:
        bool deferDeinit = true;
        GLuint id = 0;

        bool checksum(unsigned int shader, uint32_t type);
    };
}
