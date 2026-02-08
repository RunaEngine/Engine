#include "Opengl/Texture.h"
#include "Utils/Logs.h"
#include <SDL3_image/SDL_image.h>

Texture::~Texture()
{
    if (Id > 0)
        Denit();
}

bool Texture::Init(const std::filesystem::path& filepath, const char* textype, GLenum slot, GLenum channels,
                   GLenum PixelType)
{
    // Assigns the type of the texture to the texture object
    TextureType = textype;

    SDL_Surface* surf = IMG_Load(filepath.string().c_str());
    if (!surf)
    {
        Logs::SdlError();
        return false;
    }

    const SDL_PixelFormatDetails* details = SDL_GetPixelFormatDetails(surf->format);
    if (!details)
    {
        Logs::Error("Failed to get texture details");
        return false;
    }

    GLenum texChannels = GL_RED;
    GLenum internalChannels = channels;
    uint8_t numChannels = (details->Rbits > 0) + (details->Gbits > 0) + (details->Bbits > 0) + (details->Abits > 0);
    switch (numChannels)
    {
    case 4:
        texChannels = GL_RGBA;
        if (internalChannels == 0)
            internalChannels = texChannels;
        break;
    case 3:
        texChannels = GL_RGB;
        if (internalChannels == 0)
        {
            internalChannels = texChannels;
        }
        else if (internalChannels == GL_ALPHA)
        {
            internalChannels = GL_RED;
        }
        else if (internalChannels == GL_RGBA)
        {
            internalChannels = GL_RGB;
        }
        break;
    default:
        internalChannels = GL_RED;
        break;
    }

    // Generates an OpenGL texture object
    glGenTextures(1, &Id);
    // Assigns the texture to a Texture Unit
    glActiveTexture(GL_TEXTURE0 + slot);
    Unit = slot;
    glBindTexture(GL_TEXTURE_2D, Id);

    // Configures the type of algorithm that is used to make the image smaller or bigger
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Configures the way the texture repeats (if it does at all)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Extra lines in case you choose to use GL_CLAMP_TO_BORDER
    // float flatColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    // glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, flatColor);

    // Assigns the image to the OpenGL Texture object
    glTexImage2D(GL_TEXTURE_2D, 0, internalChannels, surf->w, surf->h, 0, texChannels, PixelType, surf->pixels);
    // Generates MipMaps
    glGenerateMipmap(GL_TEXTURE_2D);

    // Deletes the image data as it is already in the OpenGL Texture object
    //stbi_image_free(bytes);
    SDL_DestroySurface(surf);

    // Unbinds the OpenGL Texture object so that it can't accidentally be modified
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

bool Texture::Init(const std::vector<uint8_t>& buf, const char* textype, GLenum slot, GLenum channels,
                   GLenum pixeltype)
{
    // Assigns the type of the texture to the texture object
    TextureType = textype;

    SDL_IOStream* io = SDL_IOFromConstMem(buf.data(), buf.size());
    if (!io)
    {
        Logs::SdlError();
        return false;
    }

    SDL_Surface* surf = IMG_Load_IO(io, true);
    if (!surf)
    {
        Logs::SdlError();
        return false;
    }

    const SDL_PixelFormatDetails* details = SDL_GetPixelFormatDetails(surf->format);
    if (!details)
    {Logs::Error("Failed to get texture details");
        return false;
    }

    GLenum texChannels = GL_RED;
    GLenum internalChannels = channels;
    uint8_t numChannels = (details->Rbits > 0) + (details->Gbits > 0) + (details->Bbits > 0) + (details->Abits > 0);
    switch (numChannels)
    {
    case 4:
        texChannels = GL_RGBA;
        if (internalChannels == 0)
            internalChannels = texChannels;
        break;
    case 3:
        texChannels = GL_RGB;
        if (internalChannels == 0)
        {
            internalChannels = texChannels;
        }
        else if (internalChannels == GL_ALPHA)
        {
            internalChannels = GL_RED;
        }
        else if (internalChannels == GL_RGBA)
        {
            internalChannels = GL_RGB;
        }
        break;
    default:
        internalChannels = GL_RED;
        break;
    }

    // Generates an OpenGL texture object
    glGenTextures(1, &Id);
    // Assigns the texture to a Texture Unit
    glActiveTexture(GL_TEXTURE0 + slot);
    Unit = slot;
    glBindTexture(GL_TEXTURE_2D, Id);

    // Configures the type of algorithm that is used to make the image smaller or bigger
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Configures the way the texture repeats (if it does at all)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Extra lines in case you choose to use GL_CLAMP_TO_BORDER
    // float flatColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    // glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, flatColor);

    // Assigns the image to the OpenGL Texture object
    glTexImage2D(GL_TEXTURE_2D, 0, internalChannels, surf->w, surf->h, 0, texChannels, pixeltype, surf->pixels);
    // Generates MipMaps
    glGenerateMipmap(GL_TEXTURE_2D);

    // Deletes the image data as it is already in the OpenGL Texture object
    //stbi_image_free(bytes);
    SDL_DestroySurface(surf);

    // Unbinds the OpenGL Texture object so that it can't accidentally be modified
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

void Texture::Denit()
{
    glDeleteTextures(1, &Id);
    Id = 0;
    TextureType = 0;
    Unit = 0;
}

void Texture::TexUnit(const Shader& shader, const char* uniform, GLuint unit)
{
    // Gets the location of the uniform
    GLuint texUni = glGetUniformLocation(shader.GetId(), uniform);
    // Shader needs to be activated before changing the value of a uniform
    shader.Use();
    // Sets the value of the uniform
    glUniform1i(texUni, unit);
}

void Texture::Bind() const
{
    glActiveTexture(GL_TEXTURE0 + Unit);
    glBindTexture(GL_TEXTURE_2D, Id);
}

void Texture::Unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

const char* Texture::GetType()
{
    return TextureType;
}

GLuint Texture::GetId()
{
    return Id;
}
