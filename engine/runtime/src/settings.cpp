#include "settings.h"
#include "opengl/render.h"
#include <algorithm>

using namespace runa::runtime;

namespace runa::runtime {
    void GameUserSettings::setVsync(EVSync value) {
        SDL_GL_SetSwapInterval((int)value);
    }

    EVSync GameUserSettings::getVsync() {
        int value = 0;
        SDL_GL_GetSwapInterval(&value);
        return (EVSync)value;
    }

    void GameUserSettings::setFramerateLimit(uint16_t value) {
        if (value == 0) {
            framerateLimit = 0;
            return;
        }

        framerateLimit = (uint16_t)std::clamp((int)value, 5, 300);
    }

    uint16_t GameUserSettings::getFramerateLimit() const
    {
        return framerateLimit;
    }
}


