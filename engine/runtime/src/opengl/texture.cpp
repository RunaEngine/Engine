#include "opengl/texture.h"
#include "utils/logs.h"
#include <SDL3_image/SDL_image.h>

namespace runa::runtime::opengl {
    Texture::~Texture() {
        if (id > 0) denit();
    }

    bool Texture::init(const char* texturefile, const char* textype, GLenum slot, GLenum format, GLenum pixeltype)
    {
        // Assigns the type of the texture to the texture object
        type = textype;

        SDL_Surface* imgSurf = IMG_Load(texturefile);
        if (imgSurf == nullptr) {
            utils::Logs::error("Failed to load texture file %s", texturefile);
            return false;
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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgSurf->w, imgSurf->h, 0, format, pixeltype, imgSurf->pixels);
        // Generates MipMaps
        glGenerateMipmap(GL_TEXTURE_2D);

        // Deletes the image data as it is already in the OpenGL Texture object
        //stbi_image_free(bytes);
        SDL_DestroySurface(imgSurf);

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
