#pragma once

#include "uv.h"

namespace runa::runtime
{
    class idle_c {
    public:
        idle_c(uv_loop_t* loop);
        ~idle_c();

        int start(uv_idle_cb cb);
        int stop();
    private:
        uv_idle_t idle;
    };

    class loop_c {
    public:
        loop_c();
        ~loop_c();


        int run(uv_run_mode mode);
        void stop();

        uv_loop_t* get() { return loop; }

    private:
        uv_loop_t* loop;
        static void close_walk_cb(uv_handle_t* handle, void* arg);
    };
}