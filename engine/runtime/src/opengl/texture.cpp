#include "opengl/texture.h"
#include "utils/logs.h"
#include <SDL3_image/SDL_image.h>

namespace runa::runtime::opengl {
    Texture::~Texture() {
        if (id > 0) denit();
    }

    bool Texture::init(const char* filepath, const char* textype, GLenum slot, GLenum channels, GLenum pixeltype)
    {
        // Assigns the type of the texture to the texture object
        type = textype;

        SDL_Surface* surf = IMG_Load(filepath);
        if (!surf) {
            utils::Logs::error("Failed to load texture file %s", filepath);
            return false;
        }

        const SDL_PixelFormatDetails* details = SDL_GetPixelFormatDetails(surf->format);
        if (!details) {
            utils::Logs::error("Failed to get texture details");
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
            if (internalChannels = 0) {
                internalChannels = texChannels;
            }
            else if (internalChannels == GL_ALPHA) {
                internalChannels = GL_RED;
            } 
            else if (internalChannels == GL_RGBA) {
                internalChannels--;
            }
            break;
        default:
            internalChannels = GL_RED;
            break;
        } 

        // Generates an OpenGL texture object
        glGenTextures(1, &id);
        // Assigns the texture to a Texture Unit
        glActiveTexture(GL_TEXTURE0 + slot);
        unit = slot;
        glBindTexture(GL_TEXTURE_2D, id);

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

    void Texture::denit()
    {
        glDeleteTextures(1, &id);
        id = 0;
        type = 0;
        unit = 0;
    }

    void Texture::texUnit(const Shader& shader, const char* uniform, GLuint unit)
    {
        // Gets the location of the uniform
        GLuint texUni = glGetUniformLocation(shader.getID(), uniform);
        // Shader needs to be activated before changing the value of a uniform
        shader.use();
        // Sets the value of the uniform
        glUniform1i(texUni, unit);
    }

    void Texture::bind() const {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, id);
    }

    void Texture::unbind() const {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    const char* Texture::getType()
    {
        return type;
    }
}
