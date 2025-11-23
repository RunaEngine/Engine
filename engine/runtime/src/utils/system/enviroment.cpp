#include "utils/system/environment.h"
#include <SDL3/SDL.h>

namespace runa::runtime {
    std::string get_env_var(const char *varname) {
        SDL_Environment *env = SDL_GetEnvironment();
        if (!env) {
            SDL_Log("Error getting environment variable: %s", SDL_GetError());
            return "";
        }
        const char *val = SDL_GetEnvironmentVariable(env, varname);
        if (!val) {
            SDL_Log("Error getting environment variable: Value is null");
            return "";
        }
        return val;
    }

    std::string get_user_name() {
#ifdef _WIN64
        return get_env_var("USERNAME");
#else
        return get_env_var("HOSTNAME");
#endif

        return "";
    }
}
