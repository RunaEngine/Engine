#pragma once

#include <string>
#include <glad/glad.h>

namespace runa::runtime::opengl {
    class Shader {
    public:
        Shader() = default;
        ~Shader();

        bool init(const char* vertexfile, const char* fragmentfile);
        void deinit();

        void use() const;
        void setUniformLocation(const char *uniform, GLuint unit) const;
        GLuint getID() const { return id; }
    private:
        GLuint id = 0;

        bool checksum(unsigned int shader, const char* type);
    };
}
