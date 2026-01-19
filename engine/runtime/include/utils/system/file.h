#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace runa::runtime {
    std::vector<uint8_t> read_file(const std::string &filepath);
    std::string read_text_file(const std::string &filepath);
    bool file_exist(const std::string &filepath);
}
