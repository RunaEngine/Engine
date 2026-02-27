#include "Vulkan/VertexBuffer.h"
#include "Utils/Logs.h"

vk::VertexInputBindingDescription VKVertex::GetBindingDescription()
{
    return { 0, sizeof(VKVertex), vk::VertexInputRate::eVertex };
}

std::array<vk::VertexInputAttributeDescription, 2> VKVertex::GetAttributeDescriptions()
{
    return {
        vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(VKVertex, Pos)),
        vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(VKVertex, Color))
    };
}

VKVertexBuffer::~VKVertexBuffer()
{
    Deinit();
}
/*
template<typename T>
void VKVertexBuffer::Init(const std::vector<VKVertex>& vertices, const std::vector<T>& indices)
{
    static_assert(std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> || std::is_same_v<T, uint32_t>,
        "Indices must be uint8_t, uint16_t, or uint32_t");

    if constexpr (std::is_same_v<T, uint8_t>) {
        IndexType = vk::IndexType::eUint8EXT;
    }
    else if constexpr (std::is_same_v<T, uint16_t>) {
        IndexType = vk::IndexType::eUint16;
    }
    else if constexpr (std::is_same_v<T, uint32_t>) {
        IndexType = vk::IndexType::eUint32;
    }

    auto& device = GPipeline->GetDevice();

    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    vk::BufferCreateInfo stagingInfo;
    stagingInfo.size = bufferSize;
    stagingInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    stagingInfo.sharingMode = vk::SharingMode::eExclusive;

    vk::raii::Buffer stagingBuffer(device, stagingInfo);
    vk::MemoryRequirements memRequirementsStaging = stagingBuffer.getMemoryRequirements();
    vk::MemoryAllocateInfo memoryAllocateInfoStaging;
    memoryAllocateInfoStaging.allocationSize = memRequirementsStaging.size;
    memoryAllocateInfoStaging.memoryTypeIndex = FindMemoryType(memRequirementsStaging.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    vk::raii::DeviceMemory stagingBufferMemory(device, memoryAllocateInfoStaging);

    stagingBuffer.bindMemory(stagingBufferMemory, 0);
    void* dataStaging = stagingBufferMemory.mapMemory(0, stagingInfo.size);
    memcpy(dataStaging, vertices.data(), stagingInfo.size);
    stagingBufferMemory.unmapMemory();

    vk::BufferCreateInfo bufferInfo;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;
    VertexBuffer = vk::raii::Buffer(device, bufferInfo);

    vk::MemoryRequirements memRequirements = VertexBuffer.getMemoryRequirements();
    vk::MemoryAllocateInfo memoryAllocateInfo;
    memoryAllocateInfo.allocationSize = memRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
    VertexBufferMemory = vk::raii::DeviceMemory(device, memoryAllocateInfo);

    VertexBuffer.bindMemory(*VertexBufferMemory, 0);

    CopyBuffer(stagingBuffer, VertexBuffer, stagingInfo.size);

    CreateIndexBuffer(indices);
}
*/

void VKVertexBuffer::Deinit()
{
    Buffer = nullptr;
    BufferMemory = nullptr;
    IndexOffset = 0;
}

void VKVertexBuffer::Bind()
{
    auto& commandBuffer = GPipeline->GetCommandBuffer();

    commandBuffer.bindVertexBuffers(0, *Buffer, { 0 });
    commandBuffer.bindIndexBuffer(*Buffer, IndexOffset, IndexType);
}

vk::IndexType VKVertexBuffer::GetIndexType()
{
    return IndexType;
}

void VKVertexBuffer::CreateVertexBuffer(const std::vector<VKVertex>& vertices)
{
    auto& device = GPipeline->GetDevice();
    /*
    vk::BufferCreateInfo bufferInfo;
    bufferInfo.size = sizeof(vertices[0]) * vertices.size();
    bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    // Create Buffer

    VertexBuffer = vk::raii::Buffer(device, bufferInfo);

    vk::MemoryRequirements memRequirements = VertexBuffer.getMemoryRequirements();
    vk::MemoryAllocateInfo memoryAllocateInfo;
    memoryAllocateInfo.allocationSize = memRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    VertexBufferMemory = vk::raii::DeviceMemory(device, memoryAllocateInfo);

    VertexBuffer.bindMemory(*VertexBufferMemory, 0);

    void* data = VertexBufferMemory.mapMemory(0, bufferInfo.size);
    memcpy(data, vertices.data(), bufferInfo.size);
    VertexBufferMemory.unmapMemory();
    */

    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    CreateBuffer(bufferSize, vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, Buffer, BufferMemory);
    void* data = BufferMemory.mapMemory(0, bufferSize);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    BufferMemory.unmapMemory();
}

void VKVertexBuffer::CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& bufferMemory)
{
    auto& device = GPipeline->GetDevice();

    vk::BufferCreateInfo bufferInfo;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    buffer = vk::raii::Buffer(device, bufferInfo);
    vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
    bufferMemory = vk::raii::DeviceMemory(device, allocInfo);
    buffer.bindMemory(*bufferMemory, 0);
}

uint32_t VKVertexBuffer::FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    auto& physicalDevice = GPipeline->GetPhysicalDevice();
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    Logs::RuntimeError("Failed to find suitable memory type");
    return 0;
}

void VKVertexBuffer::CopyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size)
{
    auto& commandPool = GPipeline->GetCommandPool();
    auto& device = GPipeline->GetDevice();
    auto& queue = GPipeline->GetQueue();

    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.commandPool = commandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = 1;
    vk::raii::CommandBuffer commandCopyBuffer = std::move(device.allocateCommandBuffers(allocInfo).front());
    vk::CommandBufferBeginInfo commandBufferBeginInfo;
    commandBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    commandCopyBuffer.begin(commandBufferBeginInfo);
    commandCopyBuffer.copyBuffer(*srcBuffer, *dstBuffer, vk::BufferCopy(0, 0, size));
    commandCopyBuffer.end();

    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &*commandCopyBuffer;

    queue.submit(submitInfo, nullptr);
    queue.waitIdle();
}
/*
template<typename T>
void VKVertexBuffer::CreateIndexBuffer(const std::vector<T>& indices)
{
    vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    vk::raii::Buffer stagingBuffer({});
    vk::raii::DeviceMemory stagingBufferMemory({});
    CreateBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

    void* data = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(data, indices.data(), (size_t)bufferSize);
    stagingBufferMemory.unmapMemory();

    CreateBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, IndexBuffer, IndexBufferMemory);

    CopyBuffer(stagingBuffer, IndexBuffer, bufferSize);
}
*/