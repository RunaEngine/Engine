#pragma once

#include <cgltf.h>
#include <vector>

namespace runa::runtime::models
{
    class glft
    {
    public:
        glft() = default;
        ~glft();

        bool init(const char* filepath);
        void deinit();

    private:
        std::vector<uint8_t> data;

        std::vector<float> getFloats(cgltf_accessor* accessor);
    };
}
