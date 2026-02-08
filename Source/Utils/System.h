#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>

    bool GetEnvVar(const char* Varname, std::string& Var);

    bool GetUserName(std::string& Username);

    bool ReadFile(const std::filesystem::path& Filepath, std::vector<uint8_t>& Data);

    bool ReadTextFile(const std::filesystem::path& Filepath, std::string& Text);

    bool FileExist(const std::filesystem::path& Filepath);

    bool DirExist(const std::filesystem::path& Filepath);

    std::filesystem::path GetHomeDir();

    std::filesystem::path GetPrefPath(const std::string &Org, const std::string &App);

    std::filesystem::path BaseDir();

    std::filesystem::path currentWorkDir();
