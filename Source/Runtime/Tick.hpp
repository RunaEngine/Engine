#pragma once

#include "Engine/Core/Object.hpp"
#include <SDL3/SDL_timer.h>
#include <cstdint>

class Tick : public Object
{
public:
    Tick() = default;

    void UpdateCurrentTick()
    {
        CurrentTickNS = SDL_GetTicksNS();
    }

    void UpdateDeltaTime()
    {
        DeltaTimeNS = SDL_GetTicksNS() - CurrentTickNS;
    }

    uint64_t ElapsedNS()
    {
        return SDL_GetTicksNS() - CurrentTickNS;
    }

    uint64_t DeltaNS()
    {
        return DeltaTimeNS;
    }

    double Delta()
    {
        return double(DeltaTimeNS) / 1000000000.0;
    }

private:
    uint64_t CurrentTickNS = 0;
    uint64_t DeltaTimeNS = 0;
};
