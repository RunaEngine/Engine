#pragma once

#include <SDL3/SDL.h>
#include <functional>

namespace runa::runtime::io
{
    enum EventMode : uint8_t {
        pool = 0,
        wait = 1,
    };

    class Event
    {
    public:
        Event() = default;

        void run(EventMode mode);

        std::function<void(SDL_Event&)> onEvent;
    private:
        SDL_Event event;
    };
}