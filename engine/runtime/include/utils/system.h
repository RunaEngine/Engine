#pragma once

#include <string>
#include <vector>

namespace runa::runtime::utils
{
    bool getEnvVar(const char* varname, std::string& var);

    bool getUserName(std::string& username);

    bool readFile(const char* filepath, std::vector<uint8_t>& data);

    bool readTextFile(const char* filepath, std::string& text);

    bool fileExist(const char* filepath);

    std::string joinPaths(const std::vector<std::string>& paths);

    void nativeSeparator(std::string& path);

    std::string getHomeDir();

    std::string getPrefPath(const std::string &org, const std::string &app);

    std::string baseDir();
}
