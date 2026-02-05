#include "io/handlers.h"
#include <utility>

namespace runa::runtime
{
    void events_c::run(run_mode_t mode)
    {
        while ((mode == POLL ? SDL_PollEvent(&e) : SDL_WaitEvent(&e)))
        {
            if (callback) callback(e);
        }
    }

    bool events_c::push(SDL_Event &user_event)
    {
        return SDL_PushEvent(&user_event);
    }

    uint32_t timer_c::add(uint32_t interval)
    {
        if (id != 0) return -1;
        id = SDL_AddTimer(interval, timer_cb, this);
        return id;
    }

    uint32_t timer_c::addNS(uint64_t interval)
    {
        if (id != 0) return 0;
        id = SDL_AddTimerNS(interval, ns_timer_cb, this);
        return id;
    }

    bool timer_c::remove()
    {
        bool result = SDL_RemoveTimer(id);
        if (result) id = 0;
        return result;
    }

    uint32_t timer_c::get_id() 
    {
        return id;
    }

    uint32_t timer_c::timer_cb(void* userdata, SDL_TimerID timerID, Uint32 interval)
    {
        auto* self = (timer_c*)userdata;
        if (self->callback) self->callback();
        return timerID;
    }

    uint64_t timer_c::ns_timer_cb(void* userdata, SDL_TimerID timerID, Uint64 interval)
    {
        auto* self = (timer_c*)userdata;
        if (self->callback) self->callback();
        return timerID;
    }
}
