#pragma once

#include "Engine/Core/Object.h"
#include "Engine/Engine.h"
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>

struct VKVertex
{
	glm::vec2 Pos;
	glm::vec3 Color;

	static vk::VertexInputBindingDescription GetBindingDescription();
	static std::array<vk::VertexInputAttributeDescription, 2> GetAttributeDescriptions();
};

class VKVertexBuffer : public Object
{
public:
    VKVertexBuffer() = default;
    ~VKVertexBuffer() override;

    template<typename T>
    void Init(const std::vector<VKVertex>& vertices, const std::vector<T>& indices)
    {
        static_assert(std::is_same_v<T, uint16_t> || std::is_same_v<T, uint32_t>,
        "Indices must be uint16_t, or uint32_t");

        if constexpr (std::is_same_v<T, uint16_t>) {
            IndexType = vk::IndexType::eUint16;
        }
        else if constexpr (std::is_same_v<T, uint32_t>) {
            IndexType = vk::IndexType::eUint32;
        }

        auto& device = GPipeline->GetDevice();

        vk::DeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
        vk::DeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
        vk::DeviceSize totalSize = vertexBufferSize + indexBufferSize;
        IndexOffset = vertexBufferSize;

        vk::BufferCreateInfo stagingInfo;
        stagingInfo.size = totalSize;
        stagingInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
        stagingInfo.sharingMode = vk::SharingMode::eExclusive;

        vk::raii::Buffer stagingBuffer(device, stagingInfo);
        vk::MemoryRequirements memRequirementsStaging = stagingBuffer.getMemoryRequirements();
        vk::MemoryAllocateInfo memoryAllocateInfoStaging;
        memoryAllocateInfoStaging.allocationSize = memRequirementsStaging.size;
        memoryAllocateInfoStaging.memoryTypeIndex = FindMemoryType(memRequirementsStaging.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        vk::raii::DeviceMemory stagingBufferMemory(device, memoryAllocateInfoStaging);

        stagingBuffer.bindMemory(stagingBufferMemory, 0);
        void* dataStaging = stagingBufferMemory.mapMemory(0, totalSize);
        memcpy(dataStaging, vertices.data(), vertexBufferSize);
        memcpy((char*)dataStaging + IndexOffset, indices.data(), indexBufferSize);
        stagingBufferMemory.unmapMemory();

        CreateBuffer(totalSize, 
                         vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, 
                         vk::MemoryPropertyFlagBits::eDeviceLocal, 
                         Buffer, BufferMemory);

        CopyBuffer(stagingBuffer, Buffer, totalSize);
    }
    void Deinit();

    void Bind();
    vk::IndexType GetIndexType();
private:
    vk::raii::Buffer Buffer = nullptr;
    vk::raii::DeviceMemory BufferMemory = nullptr;
    vk::DeviceSize IndexOffset = 0;
    vk::IndexType IndexType = vk::IndexType::eUint16;

    void CreateVertexBuffer(const std::vector<VKVertex>& vertices);
    void CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& bufferMemory);
    uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    void CopyBuffer(vk::raii::Buffer & srcBuffer, vk::raii::Buffer & dstBuffer, vk::DeviceSize size);
};