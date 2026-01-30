#include "utils/system.h"
#include "utils/logs.h"
#include <SDL3/SDL.h>

namespace runa::runtime::utils
{
    bool getEnvVar(const char* varname, std::string& var)
    {
        SDL_Environment* env = SDL_GetEnvironment();
        if (!env) {
            Logs::sdlError();
            return false;
        }
        const char *val = SDL_GetEnvironmentVariable(env, varname);
        if (!val) {
            Logs::error("Error trying to find envvar %s", varname);
            return false;
        }
        var = val;
        return true;
    }

    bool getUserName(std::string& username)
    {
#ifdef _WIN64
        return getEnvVar("USERNAME", username);
#else
        return getEnvVar("HOSTNAME", username);
#endif

        return false;
    }

    bool readFile(const std::filesystem::path& filepath, std::vector<uint8_t>& data) {
        SDL_IOStream *file = SDL_IOFromFile(filepath.string().c_str(), "rb");
        if (!file) {
            Logs::sdlError();
            return false;
        }

        if (SDL_SeekIO(file, 0, SDL_IO_SEEK_END) < 0) {
            Logs::sdlError();
            SDL_CloseIO(file);
            return false;
        }

        Sint64 filesize = SDL_TellIO(file);
        if (filesize < 0) {
            Logs::sdlError();
            SDL_CloseIO(file);
            return false;
        }

        if (SDL_SeekIO(file, 0, SDL_IO_SEEK_SET) < 0) {
            SDL_Log("Error loading file to string: Unable to seek to beginning of file '%s': %s", filepath.string().c_str(), SDL_GetError());
            SDL_CloseIO(file);
            return false;
        }

        //char *buffer = SDL_malloc(filesize + 1);
        data.resize(filesize);

        size_t bytes_read = SDL_ReadIO(file, data.data(), filesize);
        SDL_CloseIO(file);

        if (bytes_read != filesize) {
            Logs::sdlError();
            data.clear(); data.shrink_to_fit();
            return false;
        }

        return true;
    }

    bool readTextFile(const std::filesystem::path& filepath, std::string& text) {
        SDL_IOStream *file = SDL_IOFromFile(filepath.string().c_str(), "rt");
        if (!file) {
            Logs::sdlError();
            return false;
        }

        if (SDL_SeekIO(file, 0, SDL_IO_SEEK_END) < 0) {
            Logs::sdlError();
            SDL_CloseIO(file);
            return false;
        }

        Sint64 filesize = SDL_TellIO(file);
        if (filesize < 0) {
            Logs::sdlError();
            SDL_CloseIO(file);
            return false;
        }

        if (SDL_SeekIO(file, 0, SDL_IO_SEEK_SET) < 0) {
            SDL_Log("Error loading file to string: Unable to seek to beginning of file '%s': %s", filepath.string().c_str(), SDL_GetError());
            SDL_CloseIO(file);
            return false;
        }

        //char *buffer = SDL_malloc(filesize + 1);
        text.resize(filesize);

        size_t bytes_read = SDL_ReadIO(file, text.data(), filesize);
        SDL_CloseIO(file);

        if (bytes_read != filesize) {
            Logs::sdlError();
            text.clear(); text.shrink_to_fit();
            return false;
        }

        return true;
    }

    bool fileExist(const std::filesystem::path &filepath) {
        if (std::filesystem::exists(filepath) && std::filesystem::is_regular_file(filepath))
        {
            return true;
        }

        return false;
    }

    bool dirExist(const std::filesystem::path& filepath)
    {
        if (std::filesystem::exists(filepath) && std::filesystem::is_directory(filepath))
        {
            return true;
        }

        return false;
    }

    std::filesystem::path getHomeDir()
    {
        std::string homeDir;
#ifdef _WIN64
        getEnvVar("USERPROFILE", homeDir);
#else
        getEnvVar("HOME", homeDir);
#endif
        return std::filesystem::path(homeDir);
    }

    std::filesystem::path getPrefPath(const std::string& org, const std::string& app)
    {
        char *path = SDL_GetPrefPath(org.c_str(), app.c_str());
        if (!path) {
            SDL_Log("Failed to get pref path");
            return "";
        }
        std::filesystem::path fs_path(path);
        SDL_free(path);
        return fs_path;
    }

    std::filesystem::path baseDir()
    {
        return std::filesystem::path(SDL_GetBasePath());
    }

    std::filesystem::path currentWorkDir()
    {
        return std::filesystem::current_path();
    }
}
