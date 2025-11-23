#pragma once
#include <cstdint>

namespace runa::runtime
{
    class timer_c
    {
    public:
        timer_c();
        ~timer_c();

        void update_current_time();
        void update_end_time();

        uint64_t elapsed_ns();
        uint64_t delta_ns();
        double delta();

    private:
        uint64_t current_time_ns = 0;
        uint64_t delta_time_ns = 0;
    };

    extern timer_c Time;
}
