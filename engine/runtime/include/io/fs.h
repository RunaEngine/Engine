#pragma once

#include <uv.h>

namespace runa::runtime
{
    class fs_c {
    public:
        fs_c();
        ~fs_c();


    private:
        uv_loop_t * loop = nullptr;
    };
}