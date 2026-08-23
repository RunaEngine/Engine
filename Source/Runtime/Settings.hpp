#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/RHI/Utils.hpp"
#include <algorithm>
#include <set>

class RHI;

enum EVSyncMode : uint8_t
{
    VSYNC_OFF = 0,
    VSYNC_ON = 1,
    VSYNC_DOUBLE_BUFFERED = 2,
    VSYNC_TRIPLE_BUFFERED = 3
};

enum EMSAACount : uint8_t
{
	MSAA_DISABLED = 0,
	MSAA_2X = 2,
    MSAA_4X = 4,
    MSAA_8X = 8
};

enum EAnisotropic : uint8_t
{
    ANISOTROPIC_DISABLED = 1,
    ANISOTROPIC_2X = 2,
    ANISOTROPIC_4X = 4,
    ANISOTROPIC_8X = 8,
    ANISOTROPIC_16X = 16,
};

class GameUserSettings : public Object
{
private:
    uint16_t FramerateLimit = 0;

    friend class RHI;
public:
    EVSyncMode VSyncMode = VSYNC_OFF;
    EAnisotropic Anisotropic = ANISOTROPIC_DISABLED;
	EMSAACount MSAACount = MSAA_DISABLED;

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


};

inline SharedPtr<GameUserSettings> GUserSettings = MakeShared<GameUserSettings>();