#include "opengl/model.h"
#include "utils/system.h"
#include "utils/logs.h"
#include <iostream>
#include <glm/gtx/matrix_decompose.hpp>


namespace runa::runtime::opengl
{
    Model::~Model()
    {
        if (deferDeinit)
            deinit();
    }

    bool Model::init(const std::filesystem::path& filepath)
    {
        Assimp::Importer importer;
        scene = importer.ReadFile(
            filepath.string().c_str(),
            aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices
        );

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            utils::Logs::error(importer.GetErrorString());
            return false;
        }

        std::filesystem::path parentpath = filepath.parent_path();
        fparentpath = &parentpath;

        constructScene(scene->mRootNode, glm::mat4(1.0f));

        scene = nullptr;
        fparentpath = nullptr;

        return true;
    }

    void Model::deinit()
    {
        for (auto& resource : assets)
        {
            resource.mesh.deinit();
        }
        assets.clear();
    }

    void Model::defer(bool value)
    {
        deferDeinit = value;
    }

    void Model::draw(Shader& shader, Camera& camera)
    {
        // Go over all meshes and draw each one
        for (auto& asset : assets)
        {
            asset.mesh.draw(
                shader,
                camera,
                asset.matrix, asset.position, asset.rotation, asset.scale
            );
        }
    }

    void Model::constructScene(aiNode* node, glm::mat4 parentMatrix)
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
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

            Asset asset;
            std::vector<Vertex> vertices;
            std::vector<GLuint> indices;
            std::vector<Texture> textures;

            // Vertex
            getVertexData(mesh, vertices, indices);
    
            // Textures
            getTextureData(scene, mesh, textures);
    
            // Mesh
            if (asset.mesh.init(vertices, indices, textures))
            {
                // Store the accumulated parent transformation (do not include this node's local transform
                // because translation/rotation/scale are passed separately to the shader). Using
                // `globalTransform` here would apply the node's local transform twice (once in the
                // matrix and once via the separate translation/rotation/scale uniforms).
                asset.matrix = globalTransform;

                // Store the full transformation matrix
                glm::vec3 translation;
                glm::quat rotation;
                glm::vec3 scale;
                glm::vec3 skew;
                glm::vec4 perspective;

                glm::decompose(globalTransform, scale, rotation, translation, skew, perspective);
        
                asset.position = translation;
                asset.rotation = rotation;
                asset.scale = scale;

                asset.mesh.defer(false);
                assets.push_back(asset);
            }
        }

        // Recursively process all children nodes with the accumulated transformation
        for (size_t i = 0; i < node->mNumChildren; i++)
        {
            aiNode* children = node->mChildren[i];
            if (!children) continue;
            constructScene(children, globalTransform);
        }
    }

    void Model::getVertexData(aiMesh* mesh, std::vector<Vertex>& vertices, std::vector<GLuint>& indices)
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

            vertex.position = glm::vec3(position.x, position.y, position.z);
            vertex.normal = glm::vec3(normal.x, normal.y, normal.z);
            vertex.color = glm::vec3(color.r, color.g, color.b);
            vertex.texUV = glm::vec2(texCoord.x, texCoord.y);

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

    void Model::getTextureData(const aiScene* scene, aiMesh* mesh, std::vector<Texture>& textures)
    {
        // Texture
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        Texture diffuse;
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
                if (diffuse.init(buffer, "diffuse", textures.size(), 0, GL_UNSIGNED_BYTE))
                {
                    diffuse.defer(false);
                }
            }
            else
            {
                // External texture
                std::filesystem::path ftexturePath = *fparentpath;
                ftexturePath.append(texturePath.C_Str());
                if (diffuse.init(ftexturePath, "diffuse", textures.size(), 0, GL_UNSIGNED_BYTE))
                {
                    diffuse.defer(false);
                }
            }
        }
        if (diffuse.getID() != 0) textures.push_back(diffuse);

        Texture specular;
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
                if (specular.init(buffer, "specular", textures.size(), 0, GL_UNSIGNED_BYTE))
                {
                    specular.defer(false);
                }
            }
            else
            {
                // External texture
                std::filesystem::path ftexturePath = *fparentpath;
                ftexturePath.append(texturePath.C_Str());
                if (specular.init(ftexturePath, "specular", textures.size(), 0, GL_UNSIGNED_BYTE))
                {
                    specular.defer(false);
                }
            }
        }
        if (specular.getID() != 0) textures.push_back(specular);
    }
}
