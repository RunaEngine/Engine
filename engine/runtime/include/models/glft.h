#pragma once

#include "opengl/vertex_array.h"
#include <cgltf.h>
#include <vector>
#include "glad/glad.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

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
        std::vector<GLuint> getIndices(cgltf_accessor* accessor);

        std::vector<opengl::Vertex> assembleVertices(std::vector<glm::vec3> positions, std::vector<glm::vec3> normals, std::vector<glm::vec2> texCoords);

        // Helps with the assembly from above by grouping floats
        std::vector<glm::vec2> groupFloatsVec2(std::vector<float> floatVec);
        std::vector<glm::vec3> groupFloatsVec3(std::vector<float> floatVec);
        std::vector<glm::vec4> groupFloatsVec4(std::vector<float> floatVec);
    };
}
