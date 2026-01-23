#include "utils/logs.h"
#include <cstdio>
#include <cstdarg>

namespace runa::runtime::utils {
    void Logs::log(SDL_PRINTF_FORMAT_STRING const char* fmt, ...)
    {
        va_list vargs;
        va_start(vargs, fmt);
        SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, vargs);
        va_end(vargs);
    }

    void Logs::success(SDL_PRINTF_FORMAT_STRING const char* fmt, ...)
    {
        va_list vargs;
        va_start(vargs, fmt);
        //SDL_Log("\033[32m");
        SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, vargs);
        //SDL_Log("\033[0m");
        va_end(vargs);
    }

    void Logs::error(SDL_PRINTF_FORMAT_STRING const char* fmt, ...)
    {
        va_list vargs;
        va_start(vargs, fmt);
        //SDL_Log("\033[31m");
        SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, fmt, vargs);
        //SDL_Log("\033[0m");
        va_end(vargs);
    }

    void Logs::warning(SDL_PRINTF_FORMAT_STRING const char* fmt, ...)
    {
        va_list vargs;
        va_start(vargs, fmt);
        printf_s("\033[33m");
        SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_WARN, fmt, vargs);
        printf_s("\033[0m");
        va_end(vargs);
    }

    void Logs::sdlError()
    {
        printf_s("\033[31m");
        SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, "%s", SDL_GetError());
        printf_s("\033[0m");
    }

    bool Logs::gltfError(cgltf_result result)
    {
        switch (result)
        {
        case cgltf_result_data_too_short:
            {
                error("GLTF import error code: 1\nData too short");
                break;
            }
        case cgltf_result_unknown_format:
            {
                error("GLTF error code: 2\nUnknown format");
                break;
            }
        case cgltf_result_invalid_json:
            {
                error("GLTF error code: 2\nInvalid json");
                break;
            }
        case cgltf_result_invalid_gltf:
            {
                error("GLTF error code: 2\nInvalid gltf");
                break;
            }
        case cgltf_result_invalid_options:
            {
                error("GLTF error code: 2\nInvalid options");
                break;
            }
        case cgltf_result_file_not_found:
            {
                error("GLTF error code: 2\nFile not found");
                break;
            }
        case cgltf_result_io_error:
            {
                error("GLTF error code: 2\nIO error");
                break;
            }
        case cgltf_result_out_of_memory:
            {
                error("GLTF error code: 2\nOut of memory");
                break;
            }
        case cgltf_result_legacy_gltf:
            {
                error("GLTF error code: 2\nLegacy gltf");
                break;
            }
        case cgltf_result_max_enum:
            {
                error("GLTF error code: 2\nMax enum");
                break;
            }
        default: break;
        }

        if (result == cgltf_result_success) return true;
        return false;
    }
}
