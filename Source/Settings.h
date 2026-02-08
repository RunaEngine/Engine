#pragma once

#include "Engine/Core/Object.h"
#include <cstdint>

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
    GameUserSettings() = default;

    void SetVsync(EVSync value);
    EVSync GetVsync();

    void SetFramerateLimit(uint16_t value);
    uint16_t GetFramerateLimit() const;

private:
    uint16_t FramerateLimit = 0;
};
