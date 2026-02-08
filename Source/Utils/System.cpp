#include "Utils/System.h"
#include "Utils/Logs.h"
#include <SDL3/SDL.h>

    bool GetEnvVar(const char* Varname, std::string& Var)
    {
        SDL_Environment* env = SDL_GetEnvironment();
        if (!env) {
            Logs::SdlError();
            return false;
        }
        const char *val = SDL_GetEnvironmentVariable(env, Varname);
        if (!val) {
            Logs::Error("Error trying to find envvar %s", Varname);
            return false;
        }
        Var = val;
        return true;
    }

    bool GetUserName(std::string& Username)
    {
#ifdef _WIN64
        return GetEnvVar("USERNAME", Username);
#else
        return GetEnvVar("HOSTNAME", Username);
#endif

        return false;
    }

    bool ReadFile(const std::filesystem::path& Filepath, std::vector<uint8_t>& Data) {
        SDL_IOStream *file = SDL_IOFromFile(Filepath.string().c_str(), "rb");
        if (!file) {
            Logs::SdlError();
            return false;
        }

        if (SDL_SeekIO(file, 0, SDL_IO_SEEK_END) < 0) {
            Logs::SdlError();
            SDL_CloseIO(file);
            return false;
        }

        Sint64 filesize = SDL_TellIO(file);
        if (filesize < 0) {
            Logs::SdlError();
            SDL_CloseIO(file);
            return false;
        }

        if (SDL_SeekIO(file, 0, SDL_IO_SEEK_SET) < 0) {
            SDL_Log("Error loading file to string: Unable to seek to beginning of file '%s': %s", Filepath.string().c_str(), SDL_GetError());
            SDL_CloseIO(file);
            return false;
        }

        //char *buffer = SDL_malloc(filesize + 1);
        Data.resize(filesize);

        size_t bytes_read = SDL_ReadIO(file, Data.data(), filesize);
        SDL_CloseIO(file);

        if (bytes_read != filesize) {
            Logs::SdlError();
            Data.clear(); Data.shrink_to_fit();
            return false;
        }

        return true;
    }

    bool ReadTextFile(const std::filesystem::path& Filepath, std::string& Text) {
        SDL_IOStream *file = SDL_IOFromFile(Filepath.string().c_str(), "rt");
        if (!file) {
            Logs::SdlError();
            return false;
        }

        if (SDL_SeekIO(file, 0, SDL_IO_SEEK_END) < 0) {
            Logs::SdlError();
            SDL_CloseIO(file);
            return false;
        }

        Sint64 filesize = SDL_TellIO(file);
        if (filesize < 0) {
            Logs::SdlError();
            SDL_CloseIO(file);
            return false;
        }

        if (SDL_SeekIO(file, 0, SDL_IO_SEEK_SET) < 0) {
            SDL_Log("Error loading file to string: Unable to seek to beginning of file '%s': %s", Filepath.string().c_str(), SDL_GetError());
            SDL_CloseIO(file);
            return false;
        }

        //char *buffer = SDL_malloc(filesize + 1);
        Text.resize(filesize);

        size_t bytes_read = SDL_ReadIO(file, Text.data(), filesize);
        SDL_CloseIO(file);

        if (bytes_read != filesize) {
            Logs::SdlError();
            Text.clear(); Text.shrink_to_fit();
            return false;
        }

        return true;
    }

    bool FileExist(const std::filesystem::path &Filepath) {
        if (std::filesystem::exists(Filepath) && std::filesystem::is_regular_file(Filepath))
        {
            return true;
        }

        return false;
    }

    bool DirExist(const std::filesystem::path& Filepath)
    {
        if (std::filesystem::exists(Filepath) && std::filesystem::is_directory(Filepath))
        {
            return true;
        }

        return false;
    }

    std::filesystem::path GetHomeDir()
    {
        std::string homeDir;
#ifdef _WIN64
        GetEnvVar("USERPROFILE", homeDir);
#else
        GetEnvVar("HOME", homeDir);
#endif
        return std::filesystem::path(homeDir);
    }

    std::filesystem::path GetPrefPath(const std::string& Org, const std::string& App)
    {
        char *path = SDL_GetPrefPath(Org.c_str(), App.c_str());
        if (!path) {
            SDL_Log("Failed to get pref path");
            return "";
        }
        std::filesystem::path fs_path(path);
        SDL_free(path);
        return fs_path;
    }

    std::filesystem::path BaseDir()
    {
        return std::filesystem::path(SDL_GetBasePath());
    }

    std::filesystem::path currentWorkDir()
    {
        return std::filesystem::current_path();
    }
