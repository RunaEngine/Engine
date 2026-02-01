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
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


namespace runa::runtime::opengl
{
    struct Asset
    {
        Mesh mesh;
        glm::vec3 position;
        glm::quat rotation;
        glm::vec3 scale;
        glm::mat4 matrix;
    };

    class Model
    {
    public:
        Model() = default;
        ~Model();

        bool init(const std::filesystem::path& filepath);
        void deinit();
        void defer(bool value = true);

        void draw(Shader& shader, Camera& camera);

    private:
        bool deferDeinit = true;
        std::vector<Asset> assets;
        std::filesystem::path* fparentpath = nullptr;

        void constructScene(aiNode* node, const aiScene* scene);
        void loadTextureFromFile(const aiTexture* texture);

        /*
        fastgltf::Expected<fastgltf::Asset>* resource = nullptr;
        std::vector<uint8_t>* bin = nullptr;
        std::filesystem::path* fparentpath = nullptr;

        std::vector<uint8_t> getData(const std::filesystem::path& parentpath);
        void traverseNode(unsigned int nextNode, glm::mat4 matrix = glm::mat4(1.0f));
        Mesh loadMesh(unsigned int indice);

        std::vector<float> getFloats(fastgltf::Accessor& accessor);
        std::vector<Texture> getTextures();
        std::vector<glm::vec3> groupFloatsVec3(std::vector<float> floatVec);
        std::vector<glm::vec2> groupFloatsVec2(std::vector<float> floatVec);
        std::vector<GLuint> getIndices(fastgltf::Accessor& accessor);

        std::vector<Vertex> assembleVertices(std::vector<glm::vec3> positions, std::vector<glm::vec3> normals, std::vector<glm::vec2> texCoords);
        */
    };
}