#pragma once

#include "Opengl/VertexArray.h"
#include "Opengl/Texture.h"
#include "Opengl/Mesh.h"
#include "Engine/Core/Object.h"
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

struct Asset
{
    SharedPtr<Mesh> Mesh;
    glm::vec3 Position;
    glm::quat Rotation;
    glm::vec3 Scale;
    glm::mat4 Matrix;
};

class Model : public Object
{
public:
    Model() = default;
    ~Model() override;

    bool Init(const std::filesystem::path& filepath);
    void Deinit();

    void Draw(Shader& shader, Camera& camera);

private:
    std::vector<Asset> Assets;
    const aiScene* Scene = nullptr;
    std::filesystem::path* ParentPath = nullptr;

    void ConstructScene(aiNode* node, glm::mat4 parentMatrix);
    void GetVertexData(aiMesh* mesh, std::vector<Vertex>& vertices, std::vector<GLuint>& indices);
    void GetTextureData(const aiScene* scene, aiMesh* mesh, std::vector<SharedPtr<Texture>>& textures);
};
