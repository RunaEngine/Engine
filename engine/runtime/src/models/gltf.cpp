#include "models/gltf.h"
#include "glad/glad.h"
#include "utils/logs.h"
#include "utils/system.h"
#include <cstddef>

namespace runa::runtime::models
{
    gltf::~gltf()
    {
        deinit();
    }

    bool gltf::init(const std::filesystem::path& filepath)
    {
        fastgltf::Parser parser;
        auto gltfBuf = fastgltf::GltfDataBuffer::FromPath(filepath);
        if (gltfBuf.error() != fastgltf::Error::None) {
            // The file couldn't be loaded, or the buffer could not be allocated.
            return false;
        }

        asset = std::make_unique<fastgltf::Expected<fastgltf::Asset>>(parser.loadGltf(gltfBuf.get(), filepath.parent_path(), fastgltf::Options::None));
        if (asset->error() != fastgltf::Error::None) {
            // Some error occurred while reading the buffer, parsing the JSON, or validating the data.
            asset.release();
            return false;
        }

        parentpath = filepath.parent_path();

        data = getData();
        traverseNode(0);
        
        return true;
    }

    void gltf::deinit()
    {
        asset.reset();
    }

    void gltf::draw(opengl::Shader& shader, opengl::Camera& camera) {
        // Go over all meshes and draw each one
        for (unsigned int i = 0; i < meshes.size(); i++)
        {
            meshes[i].draw(shader, camera, matricesMeshes[i]);
        }
    }

    void gltf::loadMesh(unsigned int indice) 
    {
        // Get all accessor indices
        fastgltf::Primitive& primitive = asset->get().meshes[indice].primitives[0];
        auto attributes = primitive.attributes;
        fastgltf::Accessor& posAccessor = asset->get().accessors.at(attributes[0].accessorIndex);
        fastgltf::Accessor& normalAccessor = asset->get().accessors.at(attributes[1].accessorIndex);
        fastgltf::Accessor& texAccessor = asset->get().accessors.at(attributes[2].accessorIndex);
        size_t indAccessor = primitive.indicesAccessor.value();

        // Use accessor indices to get all vertices components
        std::vector<float> posVec = getFloats(posAccessor);
        std::vector<glm::vec3> positions = groupFloatsVec3(posVec);
        std::vector<float> normalVec = getFloats(normalAccessor);
        std::vector<glm::vec3> normals = groupFloatsVec3(normalVec);
        std::vector<float> texVec = getFloats(texAccessor);
        std::vector<glm::vec2> texUVs = groupFloatsVec2(texVec);

        // Combine all the vertex components and also get the indices and textures
        std::vector<opengl::Vertex> vertices = assembleVertices(positions, normals, texUVs);
        std::vector<GLuint> indices = getIndices(asset->get().accessors[indAccessor]);
        std::vector<opengl::Texture> textures = getTextures();

        // Combine the vertices, indices, and textures into a mesh
        opengl::Mesh mesh;
        if (mesh.init(vertices, indices, textures)) {
            meshes.push_back(mesh);
        }
    }

    void gltf::traverseNode(unsigned int nextNode, glm::mat4 matrix) {
        // Get current node from asset data
        fastgltf::Node& node = asset->get().nodes[nextNode];

        // Get translation if it exists
        glm::vec3 translation = glm::vec3(0.0f, 0.0f, 0.0f);

        // Get quaternion if it exists
        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

        // Get scale if it exists
        glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);

        std::visit([&](auto&& arg) -> void {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, fastgltf::TRS>) {
                // TRS
                translation = glm::vec3(arg.translation.x(), arg.translation.y(), arg.translation.z());
                rotation = glm::quat(arg.rotation.w(), arg.rotation.x(), arg.rotation.y(), arg.rotation.z());
                scale = glm::vec3(arg.scale.x(), arg.scale.y(), arg.scale.z());
            }
        }, node.transform);

        // Get matrix if it exists
        auto matNode = glm::mat4(1.0f);
        std::visit([&](auto&& arg) -> void {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, fastgltf::math::fmat4x4>) {
                // Matrix
                matNode = glm::mat4(
                    arg[0][0],  arg[0][1],  arg[0][2],  arg[0][3],
                    arg[1][0],  arg[1][1],  arg[1][2],  arg[1][3],
                    arg[2][0],  arg[2][1],  arg[2][2],  arg[2][3],
                    arg[3][0],  arg[3][1],  arg[3][2],  arg[3][3]
                );

            }
        }, node.transform);

        // Initialize matrices
        glm::mat4 trans = glm::mat4(1.0f);
        glm::mat4 rot = glm::mat4(1.0f);
        glm::mat4 sca = glm::mat4(1.0f);

        // Use translation, rotation, and scale to change the initialized matrices
        trans = glm::translate(trans, translation);
        rot = glm::mat4_cast(rotation);
        sca = glm::scale(sca, scale);

        // Multiply all matrices together
        glm::mat4 matNextNode = matrix * matNode * trans * rot * sca;

        // Check if the node contains a mesh and if it does load it
        if (node.meshIndex.has_value())
        {
            translationsMeshes.push_back(translation);
            rotationsMeshes.push_back(rotation);
            scalesMeshes.push_back(scale);
            matricesMeshes.push_back(matNextNode);

            // Get the mesh index from the node
            unsigned int meshIndex = node.meshIndex.value();
            loadMesh(meshIndex);
        }

        // Check if the node has children, and if it does, apply this function to them with the matNextNode
        if (!node.children.empty())
        {
            for (unsigned int childIndex : node.children)
            {
                traverseNode(childIndex, matNextNode);
            }
        }
    }

    std::vector<uint8_t> gltf::getData() {
        std::vector<uint8_t> gltfData;

        // Get data from buffer or .bin file
        if (!asset->get().buffers.empty())
        {
            fastgltf::Buffer& buffer = asset->get().buffers[0];
            std::visit([&](auto&& source) -> void {
                using T = std::decay_t<decltype(source)>;

                if constexpr (std::is_same_v<T, fastgltf::sources::Vector>) {
                    std::vector<std::byte> bytes = source.bytes;
                    gltfData.reserve(bytes.size());
                    std::transform(source.bytes.begin(), source.bytes.end(), gltfData.begin(),
                        [](std::byte b) { return static_cast<uint8_t>(b); });
                }
                else if constexpr (std::is_same_v<T, fastgltf::sources::URI>) {
                    fastgltf::URI uri = source.uri;
                    std::vector<uint8_t> data;
                    if (utils::readFile(parentpath / uri.fspath(), data))
                    {
                        gltfData = data;
                    }
                }
            }, buffer.data);
        }

        return gltfData;
    }

    std::vector<float> gltf::getFloats(fastgltf::Accessor& accessor)
    {
        std::vector<float> floatValues;
        
        // Get properties from the accessor
        const size_t elementCount = accessor.count;
        const fastgltf::AccessorType accessorType = accessor.type;

        // Interpret the type and store it into componentsPerElement
        unsigned int componentsPerElement = 0;
        switch (accessorType)
        {
        case fastgltf::AccessorType::Scalar: componentsPerElement = 1;
            break;
        case fastgltf::AccessorType::Vec2: componentsPerElement = 2;
            break;
        case fastgltf::AccessorType::Vec3: componentsPerElement = 3;
            break;
        case fastgltf::AccessorType::Vec4: componentsPerElement = 4;
            break;
        default: break;
        }

        // Pre-allocate the vector with the exact number of floats needed
        const size_t totalFloatCount = elementCount * componentsPerElement;
        floatValues.reserve(totalFloatCount);

        // Get the actual buffer data from the accessor
        const auto& bufferView = asset->get().bufferViews[accessor.bufferViewIndex.value()];
        const size_t byteOffset = bufferView.byteOffset;
        const size_t accByteOffset = accessor.byteOffset;
        const size_t beginningOfData = byteOffset + accByteOffset;
        const size_t lengthOfData = totalFloatCount * sizeof(float);

        // Bounds check
        if (beginningOfData + lengthOfData > data.size())
        {
            // Handle error - buffer overflow would occur
            return floatValues;
        }

        // Extract floats
        for (size_t i = 0; i < totalFloatCount; ++i)
        {
            const size_t offset = beginningOfData + (i * sizeof(float));
            float value;
            memcpy(&value, &data[offset], sizeof(float));
            floatValues.push_back(value);
        }

        return floatValues;
    }

    std::vector<GLuint> gltf::getIndices(fastgltf::Accessor& accessor)
    {
        std::vector<GLuint> indices;
        
        // Get properties from the accessor
        const size_t elementCount = accessor.count;
        indices.reserve(elementCount);
        
        // Get the actual buffer data from the accessor
        const auto& bufferView = asset->get().bufferViews[accessor.bufferViewIndex.value()];
        const size_t byteOffset = bufferView.byteOffset;
        const size_t accByteOffset = accessor.byteOffset;
        const size_t beginningOfData = byteOffset + accByteOffset;
        
        // Read indices based on component type
        if (accessor.componentType == fastgltf::ComponentType::UnsignedInt)
        {
            const size_t lengthOfData = elementCount * sizeof(uint32_t);
            if (beginningOfData + lengthOfData > data.size())
            {
                return indices; // Bounds check failed
            }
            
            for (size_t i = 0; i < elementCount; ++i)
            {
                const size_t offset = beginningOfData + (i * sizeof(uint32_t));
                uint32_t value;
                memcpy(&value, &data[offset], sizeof(uint32_t));
                indices.push_back(static_cast<GLuint>(value));
            }
        }
        else if (accessor.componentType == fastgltf::ComponentType::UnsignedShort)
        {
            const size_t lengthOfData = elementCount * sizeof(uint16_t);
            if (beginningOfData + lengthOfData > data.size())
            {
                return indices; // Bounds check failed
            }
            
            for (size_t i = 0; i < elementCount; ++i)
            {
                const size_t offset = beginningOfData + (i * sizeof(uint16_t));
                uint16_t value;
                memcpy(&value, &data[offset], sizeof(uint16_t));
                indices.push_back(static_cast<GLuint>(value));
            }
        }
        else if (accessor.componentType == fastgltf::ComponentType::UnsignedByte)
        {
            const size_t lengthOfData = elementCount * sizeof(uint8_t);
            if (beginningOfData + lengthOfData > data.size())
            {
                return indices; // Bounds check failed
            }
            
            for (size_t i = 0; i < elementCount; ++i)
            {
                const size_t offset = beginningOfData + i;
                indices.push_back(static_cast<GLuint>(data[offset]));
            }
        }
        
        return indices;
    }

    std::vector<opengl::Texture> gltf::getTextures()
    {
        std::vector<opengl::Texture> textures;

        // Go over all images
        for (size_t i = 0; i < asset->get().images.size(); i++)
        {
            // Get URI of current texture
            std::string texUri;
            if (auto* uri = std::get_if<fastgltf::sources::URI>(&asset->get().images[i].data))
            {
                texUri = uri->uri.string();
            }
            else
            {
                continue;
            }

            // Check if the texture has already been loaded
            if (modelTextures.contains(texUri.c_str())) {
                textures.push_back(modelTextures.at(texUri.c_str()));
                continue;
            }

            // Load diffuse texture
            if (texUri.find("baseColor") != std::string::npos)
            {
                opengl::Texture diffuse;
                if (diffuse.init(parentpath.string() + "/" + texUri, "diffuse", modelTextures.size(), 0, GL_UNSIGNED_BYTE)) {
                    textures.push_back(diffuse);
                    modelTextures.insert_or_assign(texUri.c_str(), diffuse);
                }
            }
            // Load specular texture
            else if (texUri.find("metallicRoughness") != std::string::npos)
            {
                opengl::Texture specular;
                if (specular.init(parentpath.string() + "/" + texUri, "specular", modelTextures.size(), 0, GL_UNSIGNED_BYTE)) {
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
