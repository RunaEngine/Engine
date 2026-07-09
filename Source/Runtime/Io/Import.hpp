#pragma once

#include "Engine/Core/Object.hpp"
#include <Runtime/Vulkan/VertexBuffer.hpp>
#include "Runtime/Utils/Logs.hpp"
#include <cgltf.h>
#include <filesystem>
#include <string>

class Import : public Object
{
private:

public:
    Import() = default;

    bool LoadAsset(const std::filesystem::path& filepath, VKVertexBuffer& vertexBuffer)
    {
        if (!IsGltfFile(filepath))
        {
            Logs::Error("File extension is not .gltf or .glb");
            return false;
        }

        cgltf_options options{};
        cgltf_data* data = nullptr;

        cgltf_result result = cgltf_parse_file(&options, filepath.string().c_str(), &data);
        if (result != cgltf_result_success)
        {
            Logs::Error("Failed to import asset");
            return false;
        }

        result = cgltf_load_buffers(&options, data, filepath.string().c_str());
    
        if (result != cgltf_result_success)
        {
            Logs::Error("Failed to load gltf binaries");
        }

        return ConstructScene(data, vertexBuffer);
    }

private:
    static bool IsGltfFile(const std::filesystem::path& filepath)
    {
        return filepath.extension() == ".gltf" || filepath.extension() == ".glb";
    }

    bool ConstructScene(cgltf_data* data, VKVertexBuffer& vertexBuffer)
    {
        std::vector<VKVertex> vertices;
        std::vector<uint32_t> indices;

        for (cgltf_size i = 0; i < data->meshes_count; ++i)
        {
            const cgltf_mesh& mesh = data->meshes[i];
            for (cgltf_size j = 0; j < mesh.primitives_count; ++j)
            {
                const cgltf_primitive& primitive = mesh.primitives[i];
                if (primitive.type != cgltf_primitive_type_triangles)
                    continue;

                GetVertexData(primitive, vertices, indices);
            }
        }

        if (vertices.size() == 0 || indices.size() == 0)
            return false;
        
        vertexBuffer.Init(vertices, indices);

        return true;
    }

    void GetVertexData(const cgltf_primitive& primitive, std::vector<VKVertex>& vertices, std::vector<uint32_t>& indices)
    {
        cgltf_accessor* accessor_pos = nullptr;
        cgltf_accessor* accessor_normal = nullptr;
        cgltf_accessor* accessor_color = nullptr;
        cgltf_accessor* accessor_uv = nullptr;

        for (cgltf_size i = 0; i < primitive.attributes_count; ++i)
        {
            const cgltf_attribute& attr = primitive.attributes[i];

            if (attr.type == cgltf_attribute_type_position) {
                accessor_pos = attr.data;
            }
            else if (attr.type == cgltf_attribute_type_normal) {
                accessor_normal = attr.data;
            }
            else if (attr.type == cgltf_attribute_type_color && attr.index == 0) {
                accessor_color = attr.data;
            }
            else if (attr.type == cgltf_attribute_type_texcoord && attr.index == 0) {
                accessor_uv = attr.data;
            }
        }

        if (!accessor_pos) return;

        cgltf_size numVertices = accessor_pos->count;
        vertices.reserve(vertices.size() + numVertices);

        for (cgltf_size index = 0; index < numVertices; index++)
        {
            VKVertex vertex;

            float pos[3];
            cgltf_accessor_read_float(accessor_pos, index, pos, 3);
            vertex.Position = glm::vec3(pos[0], pos[1], pos[2]);

            if (accessor_normal) {
                float norm[3];
                cgltf_accessor_read_float(accessor_normal, index, norm, 3);
                vertex.Normal = glm::vec3(norm[0], norm[1], norm[2]);
            }
            else {
                vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            if (accessor_color) {
                float col[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
                cgltf_accessor_read_float(accessor_color, index, col, cgltf_num_components(accessor_color->type));
                vertex.Color = glm::vec3(col[0], col[1], col[2]);
            }
            else {
                vertex.Color = glm::vec3(1.0f);
            }

            if (accessor_uv) {
                float uv[2];
                cgltf_accessor_read_float(accessor_uv, index, uv, 2);
                vertex.TexCoord = glm::vec2(uv[0], uv[1]);
            }
            else {
                vertex.TexCoord = glm::vec2(0.0f);
            }

            vertices.push_back(vertex);
        }

        if (primitive.indices)
        {
            cgltf_size numIndices = primitive.indices->count;
            indices.reserve(indices.size() + numIndices);

            for (cgltf_size index = 0; index < numIndices; index++)
            {
                uint32_t indice = static_cast<uint32_t>(cgltf_accessor_read_index(primitive.indices, index));
                indices.push_back(indice);
            }
        }
        else
        {
            indices.reserve(indices.size() + numVertices);
            for (cgltf_size index = 0; index < numVertices; index++)
            {
                indices.push_back(static_cast<uint32_t>(index));
            }
        }
    }
};
