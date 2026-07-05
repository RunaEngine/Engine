#pragma once

#include "Engine/Core/Object.hpp"
#include <algorithm>

enum EWindowMode : uint8_t
{
    Fullscreen = 0,
    Windowed = 1
};

enum EMSAASample : uint8_t
{
    Disabled = 1,
    MSAA2x = 2,
    MSAA4X = 4,
    MSAA8X = 8,
    MSAA16X = 16
};

class GameUserSettings : public Object
{
public:
    bool UseVsync = false;
    EMSAASample MSAASamples = Disabled;

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

