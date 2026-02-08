#include "Settings.h"
#include "Opengl/Render.h"
#include <algorithm>

void GameUserSettings::SetVsync(EVSync value)
{
    SDL_GL_SetSwapInterval((int)value);
}

EVSync GameUserSettings::GetVsync()
{
    int value = 0;
    SDL_GL_GetSwapInterval(&value);
    return (EVSync)value;
}

void GameUserSettings::SetFramerateLimit(uint16_t value)
{
    if (value == 0)
    {
        FramerateLimit = 0;
        return;
    }

    FramerateLimit = (uint16_t)std::clamp((int)value, 5, 300);
}

uint16_t GameUserSettings::GetFramerateLimit() const
{
    return FramerateLimit;
}
