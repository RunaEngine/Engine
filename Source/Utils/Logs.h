#pragma once

#include <SDL3/SDL.h>

class Logs
{
public:
    static void Log(SDL_PRINTF_FORMAT_STRING const char* fmt, ...);

    static void Success(SDL_PRINTF_FORMAT_STRING const char* fmt, ...);

    static void Error(SDL_PRINTF_FORMAT_STRING const char* fmt, ...);

    static void Warning(SDL_PRINTF_FORMAT_STRING const char* fmt, ...);

    static void SdlError();

    //static bool gltfError(cgltf_result result);
};
