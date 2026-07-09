#pragma once

#include "Engine/Core/Object.hpp"
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>
#include <functional>

class Timer : public Object
{
public:
    Timer() = default;

    uint32_t Add(uint32_t interval)
    {
        if (id != 0) return -1;
        id = SDL_AddTimer(interval, TimerCb, this);
        return id;
    }

    uint32_t AddNs(uint64_t interval)
    {
        if (id != 0) return 0;
        id = SDL_AddTimerNS(interval, NsTimerCb, this);
        return id;
    }


    bool Remove()
    {
        bool result = SDL_RemoveTimer(id);
        if (result) id = 0;
        return result;
    }

    uint32_t GetId()
    {
        return id;
    }

    std::function<void()> OnTimer;

private:
    SDL_TimerID id = 0;

    static uint32_t TimerCb(void* userdata, SDL_TimerID timerId, Uint32 interval)
    {
        auto* self = (Timer*)userdata;
        if (self->OnTimer) self->OnTimer();
        return timerId;
    }
    static uint64_t NsTimerCb(void* userdata, SDL_TimerID timerId, Uint64 interval)
    {
        auto* self = (Timer*)userdata;
        if (self->OnTimer) self->OnTimer();
        return timerId;
    }
};
