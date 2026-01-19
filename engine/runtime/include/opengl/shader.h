#pragma once

#include <string>
#include <glad/glad.h>

namespace runa::runtime {
    class shader_c {
    public:
        shader_c(const std::string &vertexfile, const std::string &fragmentfile);
        ~shader_c();

        void use() const;
        void set_uniform_location(const char *uniform, const GLuint unit) const;
        GLuint get_id() const { return id; }
    private:
        GLuint id;
    };
}
