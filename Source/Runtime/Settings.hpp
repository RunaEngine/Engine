#pragma once

#include "Engine/Core/Object.hpp"
#include <algorithm>
#include <set>

class RHI;

enum EWindowMode : uint8_t
{
    eFullscreen = 0,
    eWindowed = 1
};

enum EAnisotropic : uint8_t
{
    eDisabled = 1,
    e2X = 1,
    e4X = 4,
    e8X = 8,
    e16X = 16,
};

class GameUserSettings : public Object
{
private:
    uint16_t FramerateLimit = 0;
    std::set<wgpu::PresentMode> SupportedPresentMode;

    friend class RHI;
public:
    wgpu::PresentMode VSync = wgpu::PresentMode::Fifo;

    bool bMSAAEnabled = false;
    EAnisotropic Anisotropic = eDisabled;
    EWindowMode WindowMode = eWindowed;

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

    std::set<wgpu::PresentMode> GetSupportedPresentMode() const
    {
        return SupportedPresentMode;
    }
};

inline SharedPtr<GameUserSettings> GUserSettings = MakeShared<GameUserSettings>();