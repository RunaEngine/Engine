#include "models/glft.h"
#include "glad/glad.h"
#include "utils/logs.h"

namespace runa::runtime::models
{
    glft::~glft()
    {
        deinit();
    }

    bool glft::init(const char* filepath)
    {
        cgltf_options options = {};
        cgltf_data* gltfData;
        if (!utils::Logs::gltfError(cgltf_parse_file(&options, filepath, &gltfData)))
        {
            return false;
        }

        //Get data fom buffer or .bin file
        cgltf_buffer* gltfBuffer = &gltfData->buffers[0];
        data.assign(
            static_cast<uint8_t*>(gltfBuffer->data),
            static_cast<uint8_t*>(gltfBuffer->data) + gltfBuffer->size
        );

        cgltf_free(gltfData);

        return true;
    }

    void glft::deinit()
    {

    }

    std::vector<float> glft::getFloats(cgltf_accessor* accessor)
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

    std::vector<GLuint> glft::getIndices(cgltf_accessor* accessor)
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

    std::vector<opengl::Vertex> glft::assembleVertices(std::vector<glm::vec3> positions, std::vector<glm::vec3> normals,
        std::vector<glm::vec2> texCoords)
    {
        std::vector<opengl::Vertex> vertices;
        vertices.reserve(positions.size());
        for (size_t i = 0; i < positions.size(); i++)
        {
            vertices.push_back(
                opengl::Vertex{ positions[i], normals[i], glm::vec3(1.0f, 1.0f, 1.0f), texCoords[i]}
                );
        }

        return vertices;
    }

    std::vector<glm::vec2> glft::groupFloatsVec2(std::vector<float> floatVec)
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

    std::vector<glm::vec3> glft::groupFloatsVec3(std::vector<float> floatVec)
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

    std::vector<glm::vec4> glft::groupFloatsVec4(std::vector<float> floatVec)
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
