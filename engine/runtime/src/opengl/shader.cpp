#include "opengl/shader.h"
#include "utils/system/file.h"
#include <SDL3/SDL.h>

namespace runa::runtime {
    shader_c::shader_c(const std::string &vertexfile, const std::string &fragmentfile) {
        id = 0;

        // Convert the shader source strings into character arrays
        std::string vertex_source = runtime::load_text_file(vertexfile);
        std::string fragment_source = runtime::load_text_file(fragmentfile);

        // Create Vertex Shader Object and get its reference
        GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        // Attach Vertex Shader source to the Vertex Shader Object
        const GLchar* vertex_src = vertex_source.c_str();
        glShaderSource(vertex_shader, 1, &vertex_src, NULL);
        // Compile the Vertex Shader into machine code
        glCompileShader(vertex_shader);

        // Create Fragment Shader Object and get its reference
        GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        // Attach Fragment Shader source to the Fragment Shader Object
        const GLchar* fragment_src = fragment_source.c_str();
        glShaderSource(fragment_shader, 1, &fragment_src, NULL);
        // Compile the Vertex Shader into machine code
        glCompileShader(fragment_shader);

        // Create Shader Program Object and get its reference
        id = glCreateProgram();
        // Attach the Vertex and Fragment Shaders to the Shader Program
        glAttachShader(id, vertex_shader);
        glAttachShader(id, fragment_shader);
        // Wrap-up/Link all the shaders together into the Shader Program
        glLinkProgram(id);

        // Delete the now useless Vertex and Fragment Shader objects
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
    }

    shader_c::~shader_c() {
        glDeleteProgram(id);
    }

    void shader_c::use() const {
        glUseProgram(id);
    }

    void shader_c::set_uniform_location(const char *uniform, const GLuint unit) const {
        // Gets the location of the uniform
        GLuint texuni = glGetUniformLocation(id, uniform);
        // Shader needs to be activated before changing the value of a uniform
        use();
        // Sets the value of the uniform
        glUniform1i(texuni, unit);
    }
}
