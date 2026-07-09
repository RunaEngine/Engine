#pragma once

#include "Runtime/Utils/Logs.hpp"
#include <SDL3/SDL.h>
#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>

inline bool GetEnvVar(const char* varname, std::string& var)
{
    SDL_Environment* env = SDL_GetEnvironment();
    if (!env)
    {
        Logs::SdlError();
        return false;
    }
    const char* val = SDL_GetEnvironmentVariable(env, varname);
    if (!val)
    {
        Logs::Error("Error trying to find envvar %s", varname);
        return false;
    }
    var = val;
    return true;
}

inline bool GetUserName(std::string& username)
{
#ifdef _WIN64
    return GetEnvVar("USERNAME", username);
#else
    return GetEnvVar("HOSTNAME", username);
#endif

    return false;
}

inline bool ReadFile(const std::filesystem::path& filepath, std::vector<uint8_t>& data)
{
    SDL_IOStream* file = SDL_IOFromFile(filepath.string().c_str(), "rb");
    if (!file)
    {
        Logs::SdlError();
        return false;
    }

    if (SDL_SeekIO(file, 0, SDL_IO_SEEK_END) < 0)
    {
        Logs::SdlError();
        SDL_CloseIO(file);
        return false;
    }

    Sint64 filesize = SDL_TellIO(file);
    if (filesize < 0)
    {
        Logs::SdlError();
        SDL_CloseIO(file);
        return false;
    }

    if (SDL_SeekIO(file, 0, SDL_IO_SEEK_SET) < 0)
    {
        SDL_Log("Error loading file to string: Unable to seek to beginning of file '%s': %s", filepath.string().c_str(),
                SDL_GetError());
        SDL_CloseIO(file);
        return false;
    }

    //char *buffer = SDL_malloc(filesize + 1);
    data.resize(filesize);

    size_t bytes_read = SDL_ReadIO(file, data.data(), filesize);
    SDL_CloseIO(file);

    if (bytes_read != filesize)
    {
        Logs::SdlError();
        data.clear();
        data.shrink_to_fit();
        return false;
    }

    return true;
}

inline bool ReadTextFile(const std::filesystem::path& filepath, std::string& text)
{
    SDL_IOStream* file = SDL_IOFromFile(filepath.string().c_str(), "rt");
    if (!file)
    {
        Logs::SdlError();
        return false;
    }

    if (SDL_SeekIO(file, 0, SDL_IO_SEEK_END) < 0)
    {
        Logs::SdlError();
        SDL_CloseIO(file);
        return false;
    }

    Sint64 filesize = SDL_TellIO(file);
    if (filesize < 0)
    {
        Logs::SdlError();
        SDL_CloseIO(file);
        return false;
    }

    if (SDL_SeekIO(file, 0, SDL_IO_SEEK_SET) < 0)
    {
        SDL_Log("Error loading file to string: Unable to seek to beginning of file '%s': %s", filepath.string().c_str(),
                SDL_GetError());
        SDL_CloseIO(file);
        return false;
    }

    //char *buffer = SDL_malloc(filesize + 1);
    text.resize(filesize);

    size_t bytes_read = SDL_ReadIO(file, text.data(), filesize);
    SDL_CloseIO(file);

    if (bytes_read != filesize)
    {
        Logs::SdlError();
        text.clear();
        text.shrink_to_fit();
        return false;
    }

    return true;
}

inline bool FileExist(const std::filesystem::path& filepath)
{
    if (std::filesystem::exists(filepath) && std::filesystem::is_regular_file(filepath))
    {
        return true;
    }

    return false;
}

inline bool DirExist(const std::filesystem::path& filepath)
{
    if (std::filesystem::exists(filepath) && std::filesystem::is_directory(filepath))
    {
        return true;
    }

    return false;
}

inline std::filesystem::path GetHomeDir()
{
    std::string homeDir;
#ifdef _WIN64
    GetEnvVar("USERPROFILE", homeDir);
#else
    GetEnvVar("HOME", homeDir);
#endif
    return std::filesystem::path(homeDir);
}

inline std::filesystem::path GetPrefPath(const std::string& org, const std::string& app)
{
    char* path = SDL_GetPrefPath(org.c_str(), app.c_str());
    if (!path)
    {
        SDL_Log("Failed to get pref path");
        return "";
    }
    std::filesystem::path fs_path(path);
    SDL_free(path);
    return fs_path;
}

inline std::filesystem::path GetBaseDir()
{
    return std::filesystem::path(SDL_GetBasePath());
}

inline std::filesystem::path currentWorkDir()
{
    return std::filesystem::current_path();
}
