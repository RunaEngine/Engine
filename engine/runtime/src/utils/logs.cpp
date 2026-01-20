#include "utils/logs.h"
#include <cstdio>
#include <cstdarg>

namespace runa::runtime::utils {
    void Logs::log(SDL_PRINTF_FORMAT_STRING const char* fmt, ...)
    {
        va_list vargs;
        va_start(vargs, fmt);
        SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, vargs);
        SDL_Log("\n");
        va_end(vargs);
    }

    void Logs::success(SDL_PRINTF_FORMAT_STRING const char* fmt, ...)
    {
        va_list vargs;
        va_start(vargs, fmt);
        SDL_Log("\033[32m");
        SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, vargs);
        SDL_Log("\033[0m\n");
        va_end(vargs);
    }

    void Logs::error(SDL_PRINTF_FORMAT_STRING const char* fmt, ...)
    {
        va_list vargs;
        va_start(vargs, fmt);
        SDL_Log("\033[31m");
        SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, fmt, vargs);
        SDL_Log("\033[0m\n");
        va_end(vargs);
    }

    void Logs::warning(SDL_PRINTF_FORMAT_STRING const char* fmt, ...)
    {
        va_list vargs;
        va_start(vargs, fmt);
        SDL_Log("\033[33m");
        SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_WARN, fmt, vargs);
        SDL_Log("\033[0m\n");
        va_end(vargs);
    }

    void Logs::sdlError()
    {
        SDL_Log("\033[31m");
        SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, "%s", SDL_GetError());
        SDL_Log("\033[0m\n");
    }
}
