#pragma once

#include "Engine/Core/Object.h"
#include <string>
#include <glad/glad.h>
#include <filesystem>

class GLShader : public Object
{
public:
    GLShader() = default;
    ~GLShader() override;

    bool Init(const std::filesystem::path& vertexfile, const std::filesystem::path& fragmentfile);
    void Deinit();
    void Defer(bool value = true);

    void Use() const;
    void SetUniformLocation(const char* uniform, GLuint unit) const;
    GLuint GetId() const { return Id; }

private:
    bool DeferDeinit = true;
    GLuint Id = 0;

    bool Checksum(unsigned int shader, uint32_t type);
};
