#include "Io/Handlers.h"
#include <utility>

void Events::run(ERunMode mode)
{
    while ((mode == Poll ? SDL_PollEvent(&e) : SDL_WaitEvent(&e)))
    {
        if (OnEvent) OnEvent(e);
    }
}

bool Events::push(SDL_Event& userEvent)
{
    return SDL_PushEvent(&userEvent);
}

uint32_t Timer::Add(uint32_t interval)
{
    if (id != 0) return -1;
    id = SDL_AddTimer(interval, TimerCb, this);
    return id;
}

uint32_t Timer::AddNs(uint64_t interval)
{
    if (id != 0) return 0;
    id = SDL_AddTimerNS(interval, NsTimerCb, this);
    return id;
}

bool Timer::Remove()
{
    bool result = SDL_RemoveTimer(id);
    if (result) id = 0;
    return result;
}

uint32_t Timer::GetId()
{
    return id;
}

uint32_t Timer::TimerCb(void* userdata, SDL_TimerID timerId, Uint32 interval)
{
    auto* self = (Timer*)userdata;
    if (self->OnTimer) self->OnTimer();
    return timerId;
}

uint64_t Timer::NsTimerCb(void* userdata, SDL_TimerID timerId, Uint64 interval)
{
    auto* self = (Timer*)userdata;
    if (self->OnTimer) self->OnTimer();
    return timerId;
}
