#include "models/glft.h"
#include "glad/glad.h"
#include "utils/logs.h"

namespace runa::runtime::models
{
    gltf::~gltf()
    {
        deinit();
    }

    bool gltf::init(const char* filepath)
    {
        cgltf_options options = {};
        if (!utils::Logs::gltfError(cgltf_parse_file(&options, filepath, &data)))
        {
            return false;
        }

        if (!utils::Logs::gltfError(cgltf_validate(data)))
        {
            cgltf_free(data);
            return false;
        }

        std::string path = filepath;
        path = path.substr(0, path.find_last_of('/') + 1);

        std::string uri = path + data->buffers[0].uri;

        if (!utils::Logs::gltfError(cgltf_load_buffers(&options, data, filepath))) {
            cgltf_free(data);
            return false;
        }

        dir = path;

        return true;
    }

    void gltf::deinit()
    {
        if (data) cgltf_free(data);
        data = nullptr;
    }

    void gltf::loadMesh(unsigned int indMesh) 
    {
        // Get all accessor indices
        cgltf_primitive* primitive = &data->meshes[indMesh].primitives[0];
        cgltf_accessor* posAccessor = primitive->attributes[0].data;
        cgltf_accessor* normalAccessor = primitive->attributes[1].data;
        cgltf_accessor* texAccessor = primitive->attributes[2].data;
        cgltf_accessor* indAccessor = primitive->indices;

        // Use accessor indices to get all vertices components
        std::vector<float> posVec = getFloats(posAccessor);
        std::vector<glm::vec3> positions = groupFloatsVec3(posVec);
        std::vector<float> normalVec = getFloats(normalAccessor);
        std::vector<glm::vec3> normals = groupFloatsVec3(normalVec);
        std::vector<float> texVec = getFloats(texAccessor);
        std::vector<glm::vec2> texUVs = groupFloatsVec2(texVec);

        // Combine all the vertex components and also get the indices and textures
        std::vector<opengl::Vertex> vertices = assembleVertices(positions, normals, texUVs);
        std::vector<GLuint> indices = getIndices(indAccessor);
        std::vector<opengl::Texture> textures = getTextures();

        // Combine the vertices, indices, and textures into a mesh
        opengl::Mesh mesh;
        if (mesh.init(vertices, indices, textures)) {
            meshes.push_back(mesh);
        }
    }

    std::vector<uint8_t> gltf::getData() {
        std::vector<uint8_t> gltfData;

        //Get data fom buffer or .bin file
        cgltf_buffer* gltfBuf = &data->buffers[0];
        gltfData.assign(
            static_cast<uint8_t*>(gltfBuf->data),
            static_cast<uint8_t*>(gltfBuf->data) + gltfBuf->size
        );

        return gltfData;
    }

    std::vector<float> gltf::getFloats(cgltf_accessor* accessor)
    {
        // Get properties from the accessor
        const cgltf_size elementCount = accessor->count;
        const cgltf_size accessorByteOffset = accessor->offset;
        const cgltf_type accessorType = accessor->type;

        // Get properties from the bufferView
        const cgltf_size bufferViewByteOffset = accessor->buffer_view->offset;

        // Interpret the type and store it into componentsPerElement
        unsigned int componentsPerElement = 0;
        switch (accessorType)
        {
        case cgltf_type_scalar: componentsPerElement = 1;
            break;
        case cgltf_type_vec2: componentsPerElement = 2;
            break;
        case cgltf_type_vec3: componentsPerElement = 3;
            break;
        case cgltf_type_vec4: componentsPerElement = 4;
            break;
        default: break;
        }

        // Pre-allocate the vector with the exact number of floats needed
        const cgltf_size totalFloatCount = elementCount * componentsPerElement;
        std::vector<float> floatValues;
        floatValues.reserve(totalFloatCount);

        // Go over all the bytes in the data at the correct place using the properties from above
        const cgltf_size dataStartOffset = bufferViewByteOffset + accessorByteOffset;
        const cgltf_size dataSizeInBytes = totalFloatCount * sizeof(float);
        for (cgltf_size byteIndex = dataStartOffset; byteIndex < dataStartOffset + dataSizeInBytes; byteIndex += sizeof(float))
        {
            float value;
            memcpy(&value, &data[byteIndex], sizeof(float));
            floatValues.push_back(value);
        }

        return floatValues;
    }

    std::vector<GLuint> gltf::getIndices(cgltf_accessor* accessor)
    {
        std::vector<GLuint> indices;

        // Get properties from the accessor
        const cgltf_size elementCount = accessor->count;
        const cgltf_size accessorByteOffset = accessor->offset;
        const cgltf_component_type componentType = accessor->component_type;

        // Get properties from the bufferView
        const cgltf_size bufferViewByteOffset = accessor->buffer_view->offset;

        // Pre-allocate the vector with the exact number of indices needed
        indices.resize(elementCount);

        // Calculate the starting position in the data buffer
        const cgltf_size dataStartOffset = bufferViewByteOffset + accessorByteOffset;

        // Extract indices based on their component type
        if (componentType == cgltf_component_type_r_32u) // Unsigned int (32-bit)
        {
            const cgltf_size dataSizeInBytes = elementCount * sizeof(unsigned int);
            cgltf_size indexPos = 0;
            for (cgltf_size byteIndex = dataStartOffset; byteIndex < dataStartOffset + dataSizeInBytes; byteIndex +=
                sizeof(unsigned int))
            {
                unsigned int value;
                memcpy(&value, &data[byteIndex], sizeof(unsigned int));
                indices[indexPos++] = (GLuint)value;
            }
        }
        else if (componentType == cgltf_component_type_r_16u) // Unsigned short (16-bit)
        {
            const cgltf_size dataSizeInBytes = elementCount * sizeof(unsigned short);
            cgltf_size indexPos = 0;
            for (cgltf_size byteIndex = dataStartOffset; byteIndex < dataStartOffset + dataSizeInBytes; byteIndex +=
                sizeof(unsigned short))
            {
                unsigned short value;
                memcpy(&value, &data[byteIndex], sizeof(unsigned short));
                indices[indexPos++] = (GLuint)value;
            }
        }
        else if (componentType == cgltf_component_type_r_16) // Signed short (16-bit)
        {
            const cgltf_size dataSizeInBytes = elementCount * sizeof(short);
            cgltf_size indexPos = 0;
            for (cgltf_size byteIndex = dataStartOffset; byteIndex < dataStartOffset + dataSizeInBytes; byteIndex +=
                sizeof(short))
            {
                short value;
                memcpy(&value, &data[byteIndex], sizeof(short));
                indices[indexPos++] = (GLuint)value;
            }
        }

        return indices;
    }

    std::vector<opengl::Texture> gltf::getTextures()
    {
        std::vector<opengl::Texture> textures;

        // Go over all images
        for (unsigned int i = 0; i < data->images_count; i++)
        {
            // uri of current texture
            std::string texUri = data->images[i].uri;

            // Check if the texture has already been loaded
            if (modelTextures.contains(texUri.c_str())) {
                textures.push_back(modelTextures.at(texUri.c_str()));
                continue;
            }
            // If the texture has been loaded, skip this

            // Load diffuse texture
            if (texUri.find("baseColor") != std::string::npos)
            {
                opengl::Texture diffuse;
                if (diffuse.init((dir + texUri).c_str(), "diffuse", modelTextures.size(), 0, GL_UNSIGNED_BYTE)) {
                    textures.push_back(diffuse);
                    modelTextures.insert_or_assign(texUri.c_str(), diffuse);
                }
            }
            // Load specular texture
            else if (texUri.find("metallicRoughness") != std::string::npos)
            {
                opengl::Texture specular;
                if (specular.init((dir + texUri).c_str(), "specular", modelTextures.size(), 0, GL_UNSIGNED_BYTE)) {
                    textures.push_back(specular);
                    modelTextures.insert_or_assign(texUri.c_str(), specular);
                }
            }
        }

        return textures;
    }

    std::vector<opengl::Vertex> gltf::assembleVertices(std::vector<glm::vec3> positions, std::vector<glm::vec3> normals,
        std::vector<glm::vec2> texCoords)
    {
        std::vector<opengl::Vertex> vertices;
        vertices.reserve(positions.size());
        for (size_t i = 0; i < positions.size(); i++)
        {
            vertices.push_back(
                opengl::Vertex{ positions[i], normals[i], glm::vec3(1.0f, 1.0f, 1.0f), texCoords[i] }
            );
        }

        return vertices;
    }

    std::vector<glm::vec2> gltf::groupFloatsVec2(std::vector<float> floatVec)
    {
        const unsigned int floatsPerVector = 2;

        std::vector<glm::vec2> vectors;
        for (unsigned int i = 0; i < floatVec.size(); i += floatsPerVector)
        {
            vectors.push_back(glm::vec2(0, 0));

            for (unsigned int j = 0; j < floatsPerVector; j++)
            {
                vectors.back()[j] = floatVec[i + j];
            }
        }
        return vectors;
    }

    std::vector<glm::vec3> gltf::groupFloatsVec3(std::vector<float> floatVec)
    {
        const unsigned int floatsPerVector = 3;

        std::vector<glm::vec3> vectors;
        for (unsigned int i = 0; i < floatVec.size(); i += floatsPerVector)
        {
            vectors.push_back(glm::vec3(0, 0, 0));

            for (unsigned int j = 0; j < floatsPerVector; j++)
            {
                vectors.back()[j] = floatVec[i + j];
            }
        }
        return vectors;
    }

    std::vector<glm::vec4> gltf::groupFloatsVec4(std::vector<float> floatVec)
    {
        const unsigned int floatsPerVector = 4;

        std::vector<glm::vec4> vectors;
        for (unsigned int i = 0; i < floatVec.size(); i += floatsPerVector)
        {
            vectors.push_back(glm::vec4(0, 0, 0, 0));

            for (unsigned int j = 0; j < floatsPerVector; j++)
            {
                vectors.back()[j] = floatVec[i + j];
            }
        }
        return vectors;
    }
}
