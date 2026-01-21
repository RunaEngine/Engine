#include "models/glft.h"

#include <vector>

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
            std::memcpy(&value, &data[byteIndex], sizeof(float));
            floatValues.push_back(value);
        }

        return floatValues;
    }
}
