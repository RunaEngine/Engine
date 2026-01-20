#pragma once

#include <cstdint>

namespace runa::runtime {
    enum EWindowMode : uint8_t {
        fullscreen = 0,
        windowed = 1
    };

    enum EVSync : int8_t {
        adaptative = -1,
        disable = 0,
        enable = 1
    };
    
    class GameUserSettings {
    public:
        GameUserSettings() = default;

        void setVsync(EVSync value);
        EVSync getVsync();
        
        void setFramerateLimit(uint16_t value);
        uint16_t getFramerateLimit() const;
    private:
        uint16_t framerateLimit = 0;
    };
}
