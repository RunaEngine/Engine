#include "opengl/shader.h"
#include "utils/logs.h"
#include "utils/system.h"
#include <SDL3/SDL.h>

namespace runa::runtime::opengl {
    Shader::~Shader() {
        if (deferDeinit && id > 0) 
            deinit();
    }

    bool Shader::init(const std::filesystem::path& vertexfile, const std::filesystem::path& fragmentfile)
    {
        // Convert the shader source strings into character arrays
        std::string vertexSource;
        if (!utils::readTextFile(vertexfile, vertexSource))
        {
            return false;
        }
        std::string fragmentSource;
        if (!utils::readTextFile(fragmentfile, fragmentSource))
        {
            return false;
        }

        // Create Vertex Shader Object and get its reference
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        // Attach Vertex Shader source to the Vertex Shader Object
        const GLchar* vertexSrc = vertexSource.c_str();
        glShaderSource(vertexShader, 1, &vertexSrc, NULL);
        // Compile the Vertex Shader into machine code
        glCompileShader(vertexShader);
        if (!checksum(vertexShader, "VERTEX"))
        {
            return false;
        }

        // Create Fragment Shader Object and get its reference
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        // Attach Fragment Shader source to the Fragment Shader Object
        const GLchar* fragmentSrc = fragmentSource.c_str();
        glShaderSource(fragmentShader, 1, &fragmentSrc, NULL);
        // Compile the Fragment Shader into machine code
        glCompileShader(fragmentShader);
        if (!checksum(fragmentShader, "FRAGMENT"))
        {
            return false;
        }

        // Create Shader Program Object and get its reference
        id = glCreateProgram();
        // Attach the Vertex and Fragment Shaders to the Shader Program
        glAttachShader(id, vertexShader);
        glAttachShader(id, fragmentShader);
        // Wrap-up/Link all the shaders together into the Shader Program
        glLinkProgram(id);
        if (!checksum(id, "PROGRAM"))
        {
            return false;
        }

        // Delete the now useless Vertex and Fragment Shader objects
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return true;
    }

    void Shader::deinit()
    {
        glDeleteProgram(id);
    }

    void Shader::defer(bool value) 
    {
        deferDeinit = value;
    }

    void Shader::use() const {
        glUseProgram(id);
    }

    void Shader::setUniformLocation(const char *uniform, const GLuint unit) const {
        // Gets the location of the uniform
        GLuint texuni = glGetUniformLocation(id, uniform);
        // Shader needs to be activated before changing the value of a uniform
        use();
        // Sets the value of the uniform
        glUniform1i(texuni, unit);
    }

    bool Shader::checksum(unsigned int shader, const char* type)
    {
        // Stores status of compilation/linking
        GLint status;
        // Character array to store error message in
        char infoLog[4096];
        // If type equals "PROGRAM" (strcmp == 0) check program link status
        if (SDL_strcmp(type, "PROGRAM") == 0)
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &status);
            if (status == GL_FALSE)
            {
                glGetProgramInfoLog(shader, 4096, NULL, infoLog);
                utils::Logs::error("SHADER_LINKING_ERROR -> %s", infoLog);
                return false;
            }
        }
        else
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
            if (status == GL_FALSE)
            {
                glGetShaderInfoLog(shader, 4096, NULL, infoLog);
                utils::Logs::error("SHADER_COMPILATION_ERROR -> %s", infoLog);
                return false;
            }
        }
        return true;
    }
}
