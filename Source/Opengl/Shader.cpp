#include "Opengl/Shader.h"
#include "Utils/Logs.h"
#include "Utils/System.h"
#include <SDL3/SDL.h>

Shader::~Shader()
{
    if (DeferDeinit && Id > 0)
        Deinit();
}

bool Shader::Init(const std::filesystem::path& vertexfile, const std::filesystem::path& fragmentfile)
{
    // Convert the shader source strings into character arrays
    std::string vertexSource;
    if (!ReadTextFile(vertexfile, vertexSource))
    {
        return false;
    }
    std::string fragmentSource;
    if (!ReadTextFile(fragmentfile, fragmentSource))
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
    if (!Checksum(vertexShader, GL_COMPILE_STATUS))
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
    if (!Checksum(fragmentShader, GL_COMPILE_STATUS))
    {
        return false;
    }

    // Create Shader Program Object and get its reference
    Id = glCreateProgram();
    // Attach the Vertex and Fragment Shaders to the Shader Program
    glAttachShader(Id, vertexShader);
    glAttachShader(Id, fragmentShader);
    // Wrap-up/Link all the shaders together into the Shader Program
    glLinkProgram(Id);
    if (!Checksum(Id, GL_LINK_STATUS))
    {
        return false;
    }

    // Delete the now useless Vertex and Fragment Shader objects
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return true;
}

void Shader::Deinit()
{
    glDeleteProgram(Id);
}

void Shader::Defer(bool value)
{
    DeferDeinit = value;
}

void Shader::Use() const
{
    glUseProgram(Id);
}

void Shader::SetUniformLocation(const char* uniform, const GLuint unit) const
{
    // Gets the location of the uniform
    GLuint texuni = glGetUniformLocation(Id, uniform);
    // Shader needs to be activated before changing the value of a uniform
    Use();
    // Sets the value of the uniform
    glUniform1i(texuni, unit);
}

bool Shader::Checksum(unsigned int shader, uint32_t type)
{
    // Stores status of compilation/linking
    int status;
    // Character array to store error message in
    char infoLog[4096];

    switch (type)
    {
    case GL_LINK_STATUS:
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &status);
            if (status == GL_FALSE)
            {
                glGetProgramInfoLog(shader, 4096, NULL, infoLog);
                Logs::Error("SHADER_LINKING_ERROR -> %s", infoLog);
                return false;
            }
            break;
        }
    default:
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
            if (status == GL_FALSE)
            {
                glGetShaderInfoLog(shader, 4096, NULL, infoLog);
                Logs::Error("SHADER_COMPILATION_ERROR -> %s", infoLog);
                return false;
            }
            break;
        }
    }
    return true;
}
