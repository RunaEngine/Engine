#pragma once

#include <SDL3/SDL.h>
#include <functional>

namespace runa::runtime::io
{
    enum EEventMode : uint8_t {
        pool = 0,
        wait = 1,
    };

    class Event
    {
    public:
        Event() = default;

        void run(EEventMode mode);

        std::function<void(SDL_Event&)> onEvent;
    private:
        SDL_Event event;
    };
}