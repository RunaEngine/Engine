#include "opengl/texture.h"
#include <stb_image.h>
#include <SDL3_image/SDL_image.h>
#include <vector>

namespace runa::runtime {
    texture_c::texture_c(const std::string &texturefile, const GLenum textype, const GLenum slot, const GLenum format,
        GLenum pixeltype) {
        id = 0;
        // Assigns the type of the texture ot the texture object
        type = textype;

        SDL_Surface *img_surf = IMG_Load(texturefile.c_str());
        if (img_surf == nullptr) {
            SDL_Log("Failed to load texture file %s", texturefile.c_str());
            return;
        }

        // Generates an OpenGL texture object
        glGenTextures(1, &id);
        // Assigns the texture to a Texture Unit
        glActiveTexture(slot);
        glBindTexture(textype, id);

        // Configures the type of algorithm that is used to make the image smaller or bigger
        glTexParameteri(textype, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        glTexParameteri(textype, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // Configures the way the texture repeats (if it does at all)
        glTexParameteri(textype, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(textype, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Extra lines in case you choose to use GL_CLAMP_TO_BORDER
        // float flatColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        // glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, flatColor);

        // Assigns the image to the OpenGL Texture object
        glTexImage2D(textype, 0, GL_RGBA, img_surf->w, img_surf->h, 0, format, pixeltype, img_surf->pixels);
        // Generates MipMaps
        glGenerateMipmap(textype);

        // Deletes the image data as it is already in the OpenGL Texture object
        //stbi_image_free(bytes);
        SDL_free(img_surf);

        // Unbinds the OpenGL Texture object so that it can't accidentally be modified
        glBindTexture(textype, 0);

        is_loaded = true;
    }

    texture_c::~texture_c() {
        glDeleteTextures(1, &id);
        type = 0;
    }

    void texture_c::bind() const {
        glBindTexture(type, id);
    }

    void texture_c::unbind() const {
        glBindTexture(type, 0);
    }
}
