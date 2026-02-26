#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>

    bool GetEnvVar(const char* varname, std::string& var);

    bool GetUserName(std::string& username);

    bool ReadFile(const std::filesystem::path& filepath, std::vector<uint8_t>& data);

    bool ReadTextFile(const std::filesystem::path& filepath, std::string& text);

    bool FileExist(const std::filesystem::path& filepath);

    bool DirExist(const std::filesystem::path& filepath);

    std::filesystem::path GetHomeDir();

    std::filesystem::path GetPrefPath(const std::string &org, const std::string &app);

    std::filesystem::path BaseDir();

    std::filesystem::path currentWorkDir();
