#pragma once
#include <cstdint>

namespace runa::runtime
{
    class Tick
    {
    public:
        Tick() = default;

        void updateCurrentTick();
        void updateDeltaTime();

        uint64_t elapsedNS();
        uint64_t deltaNS();
        double delta();

    private:
        uint64_t currentTickNS = 0;
        uint64_t deltaTimeNS = 0;
    };
}
