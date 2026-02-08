#include "Tick.h"
#include <SDL3/SDL_timer.h>

void Tick::UpdateCurrentTick()
{
    CurrentTickNS = SDL_GetTicksNS();
}

void Tick::UpdateDeltaTime()
{
    DeltaTimeNS = SDL_GetTicksNS() - CurrentTickNS;
}

uint64_t Tick::ElapsedNS()
{
    return SDL_GetTicksNS() - CurrentTickNS;
}

uint64_t Tick::DeltaNS()
{
    return DeltaTimeNS;
}

double Tick::Delta()
{
    return double(DeltaTimeNS) / 1000000000.0;
}
