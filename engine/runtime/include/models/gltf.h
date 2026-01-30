#pragma once

#include "opengl/vertex_array.h"
#include "opengl/texture.h"
#include "opengl/mesh.h"
#include <glad/glad.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <vector>
#include <map>
#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>

namespace runa::runtime::models
{
    class gltf
    {
    public:
        gltf() = default;
        ~gltf();

        bool init(const std::filesystem::path& filepath);
        void deinit();

        void draw(opengl::Shader& shader, opengl::Camera& camera);

    private:
        std::unique_ptr<fastgltf::Expected<fastgltf::Asset>> asset;
        std::vector<uint8_t> data;
        std::filesystem::path parentpath;

        std::map<const char*, opengl::Texture> modelTextures;
        std::vector<opengl::Mesh> meshes;
        std::vector<glm::vec3> translationsMeshes;
        std::vector<glm::quat> rotationsMeshes;
        std::vector<glm::vec3> scalesMeshes;
        std::vector<glm::mat4> matricesMeshes;

        void loadMesh(unsigned int indMesh);
	    void traverseNode(unsigned int nextNode, glm::mat4 matrix = glm::mat4(1.0f));
	    std::vector<uint8_t> getData();
        std::vector<float> getFloats(fastgltf::Accessor& accessor);
        std::vector<GLuint> getIndices(fastgltf::Accessor& accessor);
        std::vector<opengl::Texture> getTextures();

        std::vector<opengl::Vertex> assembleVertices(std::vector<glm::vec3> positions, std::vector<glm::vec3> normals, std::vector<glm::vec2> texCoords);

        // Helps with the assembly from above by grouping floats
        std::vector<glm::vec2> groupFloatsVec2(std::vector<float> floatVec);
        std::vector<glm::vec3> groupFloatsVec3(std::vector<float> floatVec);
        std::vector<glm::vec4> groupFloatsVec4(std::vector<float> floatVec);
    };
}
