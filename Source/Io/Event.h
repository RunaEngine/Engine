#pragma once

#include "Engine/Core/Object.h"
#include <SDL3/SDL.h>
#include <functional>

enum EEventMode : uint8_t
{
    EPool = 0,
    EWait = 1,
};

class Event : public Object
{
public:
    Event() = default;

    void Run(EEventMode mode);

    std::function<void(SDL_Event&)> OnEvent;

private:
    SDL_Event e;
};
