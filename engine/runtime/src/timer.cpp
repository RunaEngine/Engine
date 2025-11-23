#include "timer.h"
#include <SDL3/SDL_timer.h>

namespace runa::runtime
{
    timer_c::timer_c()
    {
    }

    timer_c::~timer_c()
    {
    }

    void timer_c::update_current_time()
    {
        current_time_ns = SDL_GetTicksNS();
    }

    void timer_c::update_end_time()
    {
        delta_time_ns = SDL_GetTicksNS() - current_time_ns;
    }

    uint64_t timer_c::elapsed_ns()
    {
        return SDL_GetTicksNS() - current_time_ns;
    }

    uint64_t timer_c::delta_ns()
    {
        return delta_time_ns;
    }

    double timer_c::delta()
    {
        return double(delta_time_ns) / 1000000000.0;
    }

    timer_c Time = timer_c();
}
