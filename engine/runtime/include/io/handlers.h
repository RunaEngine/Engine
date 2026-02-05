#pragma once

#include <SDL3/SDL.h>
#include <functional>
#include <memory>

namespace runa::runtime
{
    enum run_mode_t : uint8_t
    {
        POLL = 0,
        WAIT
    };

    class events_c
    {
    public:
        events_c() = default;

        void run(run_mode_t mode);

        bool push(SDL_Event &user_event);

        std::function<void(SDL_Event&)> callback;
    private:
        SDL_Event e;
    };

    class timer_c
    {
    public:
        timer_c() = default;    

        uint32_t add(uint32_t interval);

        uint32_t addNS(uint64_t interval);

        bool remove();

        uint32_t get_id();

        std::function<void()> callback;
    private:
        SDL_TimerID id = 0;

        static uint32_t timer_cb(void *userdata, SDL_TimerID timerID, Uint32 interval);
        static uint64_t ns_timer_cb(void *userdata, SDL_TimerID timerID, Uint64 interval);
    };
}