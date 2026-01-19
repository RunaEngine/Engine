#include "utils/system/path.h"
#include "utils/system/environment.h"
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

namespace runa::runtime {
    std::string join_paths(const std::vector<std::string> &paths) {
        if (paths.empty()) {
            SDL_Log("Paths array is NULL");
            return "";
        }

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

    void native_separator(std::string &path) {
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

    std::string get_home_dir() {
#ifdef _WIN64
        return get_env_var("USERPROFILE");
#else
        return get_env_var("HOME");
#endif
        return "";
    }

    std::string get_pref_path(const std::string &org, const std::string &app) {
        char *path = SDL_GetPrefPath(org.c_str(), app.c_str());
        if (!path) {
            SDL_Log("Failed to get pref path");
            return "";
        }
        std::string path_str(path);
        SDL_free(path);
        return path_str;
    }

    std::string current_dir() {
        return SDL_GetBasePath();
    }
}

