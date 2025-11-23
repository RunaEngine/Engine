#pragma once
#include <string>
#include <vector>

namespace runa::runtime {
    std::string join_paths(const std::vector<std::string> &paths);
    void native_separator(std::string &path);
    std::string get_home_dir();
    std::string get_pref_path(const std::string &org, const std::string &app);
    std::string current_dir();
}
