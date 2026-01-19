#pragma once

#include <string>

namespace runa::runtime {
    std::string get_env_var(const char *varname);
    std::string get_user_name();
}
