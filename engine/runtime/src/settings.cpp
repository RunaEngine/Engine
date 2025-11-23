#include "settings.h"
#include "opengl/render.h"
#include <algorithm>

using namespace runa::runtime;

namespace runa::runtime {
    game_user_settings_c::game_user_settings_c()
    = default;

    game_user_settings_c::~game_user_settings_c()
    = default;

    void game_user_settings_c::set_vsync(vsync_e value) {
        SDL_GL_SetSwapInterval((int)value);
    }

    vsync_e game_user_settings_c::get_vsync() {
        int value = 0;
        SDL_GL_GetSwapInterval(&value);
        return (vsync_e)value;
    }

    void game_user_settings_c::set_framerate_limit(uint16_t value) {
        if (value == 0) {
            framerate_limit = 0;
            return;
        }

        framerate_limit = (uint16_t)std::clamp((int)value, 5, 300);
    }

    uint16_t game_user_settings_c::get_framerate_limit() const
    {
        return framerate_limit;
    }

    game_user_settings_c GameUserSettings = game_user_settings_c();
}


