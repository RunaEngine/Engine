#pragma once

#include "Engine/Core/Object.hpp"
#include <algorithm>

enum EWindowMode : uint8_t
{
    Fullscreen = 0,
    Windowed = 1
};

enum EVSync : int8_t
{
    Adaptative = -1,
    Disable = 0,
    Enable = 1
};

class GameUserSettings : public Object
{
public:
    bool UseVsync = false;

    GameUserSettings() = default;

    void SetFramerateLimit(uint16_t value)
    {
        if (value == 0)
        {
            FramerateLimit = 0;
            return;
        }

        FramerateLimit = (uint16_t)std::clamp((int)value, 5, 300);
    }

    uint16_t GetFramerateLimit() const
    {
        return FramerateLimit;
    }

private:
    uint16_t FramerateLimit = 0;
};

