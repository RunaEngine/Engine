#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/RHI/Utils.hpp"
#include <algorithm>
#include <set>

class RHI;

enum EVSyncMode : uint8_t
{
    VSYNCOFF = 0,
    VSYNCON = 1,
    VSYNCDOUBLEBUFFERED = 2,
    VSYNCTRIPLEBUFFERED = 3
};

class GameUserSettings : public Object
{
private:
    uint16_t FramerateLimit = 0;

    friend class RHI;
public:
    EVSyncMode VSyncMode = VSYNCOFF;

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