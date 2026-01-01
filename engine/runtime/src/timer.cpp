#include "timer.h"
#include <SDL3/SDL_timer.h>

namespace runa::runtime
{
    time_c::time_c()
    {
    }

    time_c::~time_c()
    {
    }

    void time_c::update_current_time()
    {
        current_time_ns = SDL_GetTicksNS();
    }

    void time_c::update_end_time()
    {
        delta_time_ns = SDL_GetTicksNS() - current_time_ns;
    }

    uint64_t time_c::elapsed_ns()
    {
        return SDL_GetTicksNS() - current_time_ns;
    }

    uint64_t time_c::delta_ns()
    {
        return delta_time_ns;
    }

    double time_c::delta()
    {
        return double(delta_time_ns) / 1000000000.0;
    }

    time_c Time = time_c();
}
