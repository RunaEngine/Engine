#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>

namespace runa::runtime::utils
{
    bool getEnvVar(const char* varname, std::string& var);

    bool getUserName(std::string& username);

    bool readFile(const std::filesystem::path& filepath, std::vector<uint8_t>& data);

    bool readTextFile(const std::filesystem::path& filepath, std::string& text);

    bool fileExist(const std::filesystem::path& filepath);

    bool dirExist(const std::filesystem::path& filepath);

    std::filesystem::path getHomeDir();

    std::filesystem::path getPrefPath(const std::string &org, const std::string &app);

    std::filesystem::path baseDir();

    std::filesystem::path currentWorkDir();
}
