#include "tick.h"
#include <SDL3/SDL_timer.h>

namespace runa::runtime
{
    void Tick::updateCurrentTick()
    {
        currentTickNS = SDL_GetTicksNS();
    }

    void Tick::updateDeltaTime()
    {
        deltaTimeNS = SDL_GetTicksNS() - currentTickNS;
    }

    uint64_t Tick::elapsedNS()
    {
        return SDL_GetTicksNS() - currentTickNS;
    }

    uint64_t Tick::deltaNS()
    {
        return deltaTimeNS;
    }

    double Tick::delta()
    {
        return double(deltaTimeNS) / 1000000000.0;
    }
}
