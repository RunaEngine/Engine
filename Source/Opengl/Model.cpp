#include "Opengl/Model.h"
#include "Utils/System.h"
#include "Utils/Logs.h"
#include <iostream>
#include <glm/gtx/matrix_decompose.hpp>


GLModel::~GLModel()
{
    Deinit();
}

bool GLModel::Init(const std::filesystem::path& filepath)
{
    Assimp::Importer importer;
    Scene = importer.ReadFile(
        filepath.string().c_str(),
        aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices
    );

    if (!Scene || Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !Scene->mRootNode)
    {
        Logs::Error(importer.GetErrorString());
        return false;
    }

    std::filesystem::path parentpath = filepath.parent_path();
    ParentPath = &parentpath;

    ConstructScene(Scene->mRootNode, glm::mat4(1.0f));

    Scene = nullptr;
    ParentPath = nullptr;

    return true;
}

void GLModel::Deinit()
{
    for (auto& resource : Assets)
    {
        if (resource.Mesh)
        {
            resource.Mesh->Deinit();
        }
    }
    Assets.clear();
}

void GLModel::Draw(GLShader& shader, GLCamera& camera)
{
    // Go over all meshes and draw each one
    for (auto& asset : Assets)
    {
        asset.Mesh->Draw(
            shader,
            camera,
            asset.Matrix, asset.Position, asset.Rotation, asset.Scale
        );
    }
}

void GLModel::ConstructScene(aiNode* node, glm::mat4 parentMatrix)
{
    // Get the node's transformation matrix
    // Assimp matrices are row-major, while GLM uses column-major matrices.
    // A direct memcpy of the data from an aiMatrix4x4 to a glm::mat4
    // effectively transposes the matrix, which is exactly what we need for correct multiplication.
    glm::mat4 nodeTransform;
    memcpy(&nodeTransform, &node->mTransformation, sizeof(aiMatrix4x4));

    // Multiply with parent matrix to get the final transformation
    glm::mat4 globalTransform = parentMatrix * nodeTransform;

    // Process all meshes in this node
    for (size_t i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = Scene->mMeshes[node->mMeshes[i]];

        Asset asset;
        asset.Mesh = MakeShared<GLMesh>();
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        std::vector<std::shared_ptr<GLTexture>> textures;

        // Vertex
        GetVertexData(mesh, vertices, indices);

        // Textures
        GetTextureData(Scene, mesh, textures);

        // Mesh
        if (asset.Mesh->Init(vertices, indices, textures))
        {
            // Store the accumulated parent transformation (do not include this node's local transform
            // because translation/rotation/scale are passed separately to the shader). Using
            // `globalTransform` here would apply the node's local transform twice (once in the
            // matrix and once via the separate translation/rotation/scale uniforms).
            asset.Matrix = globalTransform;

            // Store the full transformation matrix
            glm::vec3 translation;
            glm::quat rotation;
            glm::vec3 scale;
            glm::vec3 skew;
            glm::vec4 perspective;

            glm::decompose(globalTransform, scale, rotation, translation, skew, perspective);

            asset.Position = translation;
            asset.Rotation = rotation;
            asset.Scale = scale;

            Assets.push_back(asset);
        }
    }

    // Recursively process all children nodes with the accumulated transformation
    for (size_t i = 0; i < node->mNumChildren; i++)
    {
        aiNode* children = node->mChildren[i];
        if (!children) continue;
        ConstructScene(children, globalTransform);
    }
}

void GLModel::GetVertexData(aiMesh* mesh, std::vector<Vertex>& vertices, std::vector<GLuint>& indices)
{
    vertices.reserve(mesh->mNumVertices);

    // Vertices
    for (size_t index = 0; index < mesh->mNumVertices; index++)
    {
        Vertex vertex;
        aiVector3D position = mesh->mVertices[index];
        aiVector3D normal = mesh->HasNormals() ? mesh->mNormals[index] : aiVector3D(0);
        aiColor4D color = mesh->HasVertexColors(0) ? mesh->mColors[0][index] : aiColor4D(1);
        aiVector3D texCoord = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][index] : aiVector3D(0);

        vertex.Position = glm::vec3(position.x, position.y, position.z);
        vertex.Normal = glm::vec3(normal.x, normal.y, normal.z);
        vertex.Color = glm::vec3(color.r, color.g, color.b);
        vertex.TexUV = glm::vec2(texCoord.x, texCoord.y);

        vertices.push_back(vertex);
    }

    // Indices
    indices.reserve(mesh->mNumFaces * 3);
    for (size_t faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++)
    {
        aiFace face = mesh->mFaces[faceIndex];
        for (size_t indIndex = 0; indIndex < face.mNumIndices; indIndex++)
        {
            GLuint indice = face.mIndices[indIndex];
            indices.push_back(indice);
        }
    }
}

void GLModel::GetTextureData(const aiScene* scene, aiMesh* mesh, std::vector<SharedPtr<GLTexture>>& textures)
{
    // Texture
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    //Texture* diffuse = nullptr;
    aiString texturePath;
    if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS)
    {
        std::string textureFileName = texturePath.C_Str();
        // Check if the texture is embedded or external
        if (textureFileName.c_str()[0] == '*')
        {
            // Embedded texture
            unsigned int textureIndex = std::stoul(textureFileName.substr(1));
            aiTexture* embeddedTexture = scene->mTextures[textureIndex];
            std::vector<uint8_t> buffer;
            if (embeddedTexture->mHeight == 0)
            {
                auto data = reinterpret_cast<uint8_t*>(embeddedTexture->pcData);
                buffer.assign(data, data + embeddedTexture->mWidth);
            }
            else
            {
                size_t size = embeddedTexture->mWidth * embeddedTexture->mHeight * sizeof(aiTexel);
                auto data = reinterpret_cast<uint8_t*>(embeddedTexture->pcData);
                buffer.assign(data, data + size);
            }
            auto diffuse = MakeShared<GLTexture>();
            if (diffuse->Init(buffer, "diffuse", textures.size(), 0, GL_UNSIGNED_BYTE))
            {
                textures.push_back(diffuse);
            }
        }
        else
        {
            // External texture
            std::filesystem::path ftexturePath = *ParentPath;
            ftexturePath.append(texturePath.C_Str());
            auto diffuse = MakeShared<GLTexture>();
            if (diffuse->Init(ftexturePath, "diffuse", textures.size(), 0, GL_UNSIGNED_BYTE))
            {
                textures.push_back(diffuse);
            }
        }
    }

    if (material->GetTexture(aiTextureType_SPECULAR, 0, &texturePath) == AI_SUCCESS)
    {
        std::string textureFileName = texturePath.C_Str();
        // Check if the texture is embedded or external
        if (textureFileName.c_str()[0] == '*')
        {
            // Embedded texture
            unsigned int textureIndex = std::stoul(textureFileName.substr(1));
            aiTexture* embeddedTexture = scene->mTextures[textureIndex];
            std::vector<uint8_t> buffer;
            if (embeddedTexture->mHeight == 0)
            {
                auto data = reinterpret_cast<uint8_t*>(embeddedTexture->pcData);
                buffer.assign(data, data + embeddedTexture->mWidth);
            }
            else
            {
                size_t size = embeddedTexture->mWidth * embeddedTexture->mHeight * sizeof(aiTexel);
                auto data = reinterpret_cast<uint8_t*>(embeddedTexture->pcData);
                buffer.assign(data, data + size);
            }
            auto specular = MakeShared<GLTexture>();
            if (specular->Init(buffer, "specular", textures.size(), 0, GL_UNSIGNED_BYTE))
            {
                textures.push_back(specular);
            }
        }
        else
        {
            // External texture
            std::filesystem::path ftexturePath = *ParentPath;
            ftexturePath.append(texturePath.C_Str());
            auto specular = MakeShared<GLTexture>();
            if (specular->Init(ftexturePath, "specular", textures.size(), 0, GL_UNSIGNED_BYTE))
            {
                textures.push_back(specular);
            }
        }
    }
}
