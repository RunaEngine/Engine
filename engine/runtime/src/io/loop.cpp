#include "io/loop.h"
#include <SDL3/SDL_log.h>

namespace runa::runtime {
    idle_c::idle_c(uv_loop_t* loop)
    {
        int status = uv_idle_init(loop, &idle);
        if (status != 0)
        {
            SDL_Log("Failed to init idle: %s", uv_strerror(status));
        }
    }

    idle_c::~idle_c()
    {
        int status = uv_idle_stop(&idle);
        if (status != 0)
        {
            SDL_Log("Failed to stop idle: %s", uv_strerror(status));
        }
    }

    int idle_c::start(uv_idle_cb cb)
    {
        return uv_idle_start(&idle, cb);
    }

    int idle_c::stop()
    {
        return uv_idle_stop(&idle);
    }

    loop_c::loop_c()
    {
        loop = uv_default_loop();
        if (!loop)
        {
            SDL_Log("Failed to create loop");
        }
    }

    loop_c::~loop_c()
    {
        uv_walk(loop, close_walk_cb, NULL);
        int status = uv_loop_close(loop);
        if (status != 0)
        {
            SDL_Log("Failed to close loop: %s", uv_strerror(status));
            return;
        }
        SDL_free(loop);
    }

    int loop_c::run(uv_run_mode mode)
    {
        return uv_run(loop, mode);
    }

    void loop_c::stop()
    {
        uv_stop(loop);
    }

    void loop_c::close_walk_cb(uv_handle_t* handle, void* arg)
    {
        if (uv_is_closing(handle)) return;
        uv_close(handle, NULL);
    }
}




