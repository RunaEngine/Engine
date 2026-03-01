#pragma once

#include <SDL3/SDL.h>
#include <cstdio>
#include <cstdarg>
#include <exception>

class Logs
{
public:
    static void Log(SDL_PRINTF_FORMAT_STRING const char* fmt, ...)
    {
        va_list vargs;
        va_start(vargs, fmt);
        SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, vargs);
        va_end(vargs);
    }

    static void Success(SDL_PRINTF_FORMAT_STRING const char* fmt, ...)
    {
        va_list vargs;
        va_start(vargs, fmt);
        printf("\033[32m");
        SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, vargs);
        printf("\033[0m");
        va_end(vargs);
    }

    static void Error(SDL_PRINTF_FORMAT_STRING const char* fmt, ...)
    {
        va_list vargs;
        va_start(vargs, fmt);
        printf("\033[31m");
        SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, fmt, vargs);
        printf("\033[0m");
        va_end(vargs);
    }

    static void Warning(SDL_PRINTF_FORMAT_STRING const char* fmt, ...)
    {
        va_list vargs;
        va_start(vargs, fmt);
        printf("\033[33m");
        SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_WARN, fmt, vargs);
        printf("\033[0m");
        va_end(vargs);
    }

    static void RuntimeError(SDL_PRINTF_FORMAT_STRING const char* fmt, ...)
    {
        va_list vargs;
        va_start(vargs, fmt);
        printf("\033[31m");
        SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, fmt, vargs);
        printf("\033[0m");
        va_end(vargs);
        std::terminate();
    }

    static void SdlError()
    {
        printf("\033[31m");
        SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, "%s", SDL_GetError());
        printf("\033[0m");
    }
};
