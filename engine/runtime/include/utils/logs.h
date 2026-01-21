#pragma once

#include <SDL3/SDL.h>
#include <cgltf.h>

namespace runa::runtime::utils {
    class Logs
    {
    public:
        static void log(SDL_PRINTF_FORMAT_STRING const char* fmt, ...);

        static void success(SDL_PRINTF_FORMAT_STRING const char *fmt, ...);

        static void error(SDL_PRINTF_FORMAT_STRING const char* fmt, ...);

        static void warning(SDL_PRINTF_FORMAT_STRING const char* fmt, ...);

        static void sdlError();

        static bool gltfError(cgltf_result result);
    };
    
}
