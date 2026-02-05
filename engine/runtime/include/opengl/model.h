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
        const aiScene* scene = nullptr;
        std::filesystem::path* fparentpath = nullptr;

        void constructScene(aiNode* node, glm::mat4 parentMatrix);
        void getVertexData(aiMesh* mesh, std::vector<Vertex>& vertices, std::vector<GLuint>& indices);
        void getTextureData(const aiScene* scene, aiMesh* mesh, std::vector<Texture>& textures);
    };
}