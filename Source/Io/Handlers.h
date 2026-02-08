#pragma once

#include "Engine/Core/Object.h"
#include <SDL3/SDL.h>
#include <functional>
#include <memory>

enum ERunMode : uint8_t
{
    Poll = 0,
    Wait = 1
};

class Events : public Object
{
public:
    Events() = default;

    void run(ERunMode mode);

    bool push(SDL_Event& userEvent);

    std::function<void(SDL_Event&)> OnEvent;

private:
    SDL_Event e;
};

class Timer : public Object
{
public:
    Timer() = default;

    uint32_t Add(uint32_t interval);

    uint32_t AddNs(uint64_t interval);

    bool Remove();

    uint32_t GetId();

    std::function<void()> OnTimer;

private:
    SDL_TimerID id = 0;

    static uint32_t TimerCb(void* userdata, SDL_TimerID timerId, Uint32 interval);
    static uint64_t NsTimerCb(void* userdata, SDL_TimerID timerId, Uint64 interval);
};
