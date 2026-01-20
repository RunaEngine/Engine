#include "utils/system.h"
#include "utils/logs.h"
#include <SDL3/SDL.h>

#ifdef _WIN64
const char PATH_SEPARATOR = '\\';
const char PATH_SEPARATOR_OTHER = '/';
const char *PATH_SEPARATOR_STR = "\\";
#else
const char PATH_SEPARATOR = '/';
const char PATH_SEPARATOR_OTHER = '\\';
const char* PATH_SEPARATOR_STR = "/";
#endif

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

    bool readFile(const char* filepath, std::vector<uint8_t>& data) {
        SDL_IOStream *file = SDL_IOFromFile(filepath, "rb");
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
            SDL_Log("Error loading file to string: Unable to seek to beginning of file '%s': %s", filepath, SDL_GetError());
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

    bool readTextFile(const char* filepath, std::string& text) {
        SDL_IOStream *file = SDL_IOFromFile(filepath, "rt");
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
            SDL_Log("Error loading file to string: Unable to seek to beginning of file '%s': %s", filepath, SDL_GetError());
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

    bool fileExist(const std::string &filepath) {
        SDL_IOStream *file = SDL_IOFromFile(filepath.c_str(), "rb");

        if (file) {
            Logs::sdlError();
            SDL_CloseIO(file);
            return true;
        }

        // No need to log here as file not existing is often an expected case
        return false;
    }

    std::string joinPaths(const std::vector<std::string>& paths)
    {
        std::string joined_path;

        for (const std::string &path : paths) {
            if (path.empty()) continue;

            size_t str_len = path.length();
            std::string str = path;
            bool was_separator = false;
            for (size_t i = 0; i < str_len; i++) {
                char &c = str[i];
                if (i == 0 && (c == PATH_SEPARATOR || c == PATH_SEPARATOR_OTHER)) {
                    was_separator = true;
                    continue;
                }
                if (c == PATH_SEPARATOR || c == PATH_SEPARATOR_OTHER) {
                    if (was_separator) continue;
                    was_separator = true;
                    c = PATH_SEPARATOR;
                }
            }
            if (str.back() != PATH_SEPARATOR) {
                str.push_back(PATH_SEPARATOR);
            }
            joined_path += str;
        }
        return joined_path;
    }

    void nativeSeparator(std::string& path)
    {
        if (path.empty()) {
            SDL_Log("Path is NULL");
        }

        size_t str_len = path.length();
        for (char &c : path) {
            if (c == PATH_SEPARATOR_OTHER) {
                c = PATH_SEPARATOR;
            }
        }
    }

    std::string getHomeDir()
    {
        std::string homeDir;
#ifdef _WIN64
        getEnvVar("USERPROFILE", homeDir);
#else
        getEnvVar("HOME", homeDir);
#endif
        return homeDir;
    }

    std::string getPrefPath(const std::string& org, const std::string& app)
    {
        char *path = SDL_GetPrefPath(org.c_str(), app.c_str());
        if (!path) {
            SDL_Log("Failed to get pref path");
            return "";
        }
        std::string path_str(path);
        SDL_free(path);
        return path_str;
    }

    std::string baseDir()
    {
        return SDL_GetBasePath();
    }
}
