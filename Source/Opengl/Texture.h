#pragma once

#include "Shader.h"
#include "Engine/Core/Object.h"
#include <glad/glad.h>
#include <filesystem>
#include <vector>

class Texture : public Object
{
public:
    Texture() = default;
    ~Texture() override;

    bool Init(const std::filesystem::path& texturefile, const char* textype, GLenum slot, GLenum channels,
              GLenum PixelType);
    bool Init(const std::vector<uint8_t>& buf, const char* textype, GLenum slot, GLenum channels, GLenum pixeltype);
    void Denit();

    void TexUnit(const Shader& shader, const char* uniform, GLuint unit);

    void Bind() const;
    void Unbind() const;

    const char* GetType();
    GLuint GetId();

private:
    bool DeferDeinit = true;
    GLuint Id = 0;
    const char* TextureType = "";
    GLuint Unit = 0;
};
