#pragma once

#include "opengl/vertex_array.h"
#include "opengl/texture.h"
#include "opengl/mesh.h"
#include <cgltf.h>
#include <glad/glad.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <vector>
#include <map>

namespace runa::runtime::models
{
    class gltf
    {
    public:
        gltf() = default;
        ~gltf();

        bool init(const char* filepath);
        void deinit();

    private:
        cgltf_data* data = nullptr;
        std::string dir;

        std::map<const char*, opengl::Texture> modelTextures;
        std::vector<opengl::Mesh> meshes;
        std::vector<glm::vec3> translationsMeshes;
        std::vector<glm::quat> rotationsMeshes;
        std::vector<glm::vec3> scalesMeshes;
        std::vector<glm::mat4> matricesMeshes;

        void loadMesh(unsigned int indMesh);
	    std::vector<uint8_t> getData();
        std::vector<float> getFloats(cgltf_accessor* accessor);
        std::vector<GLuint> getIndices(cgltf_accessor* accessor);
        std::vector<opengl::Texture> getTextures();

        std::vector<opengl::Vertex> assembleVertices(std::vector<glm::vec3> positions, std::vector<glm::vec3> normals, std::vector<glm::vec2> texCoords);

        // Helps with the assembly from above by grouping floats
        std::vector<glm::vec2> groupFloatsVec2(std::vector<float> floatVec);
        std::vector<glm::vec3> groupFloatsVec3(std::vector<float> floatVec);
        std::vector<glm::vec4> groupFloatsVec4(std::vector<float> floatVec);
    };
}
