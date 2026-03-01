#pragma once

#include "Vulkan/Pipeline.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Core/Object.hpp"
#include "Utils/Logs.hpp"
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

struct VKVertex
{
	glm::vec2 Pos;
	glm::vec3 Color;

	static vk::VertexInputBindingDescription GetBindingDescription()
	{
	    return { 0, sizeof(VKVertex), vk::VertexInputRate::eVertex };
	}
	static std::array<vk::VertexInputAttributeDescription, 2> GetAttributeDescriptions()
	{
	    return {
	        vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(VKVertex, Pos)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(VKVertex, Color))
        };
	}
};

struct VKUniformBuffer {
    alignas(16) glm::mat4 Model;
    alignas(16) glm::mat4 View;
    alignas(16) glm::mat4 Proj;
};

class VKVertexBuffer : public Object
{
public:
    VKVertexBuffer() = default;
    ~VKVertexBuffer() override
    {
        Deinit();
    }

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

        CreateDescriptorSetLayout();

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

        // Uniform buffer object
        CreateUniformBuffers();
        CreateDescriptorPool();
        CreateDescriptorSets();
    }
    void Deinit()
    {
        DescriptorSets.clear();
        DescriptorPool = nullptr;
        UniformBuffers.clear();
        UniformBuffersMemory.clear();
        UniformBuffersMapped.clear();
        DescriptorSetLayout = nullptr;
        BufferMemory = nullptr;
        Buffer = nullptr;
        IndexOffset = 0;
        IndexType = vk::IndexType::eUint16;
    }

    void Bind(uint32_t frameIndex, vk::raii::PipelineLayout& pipelineLayout)
    {
        auto& commandBuffer = GPipeline->GetCommandBuffer();

        commandBuffer.bindVertexBuffers(0, *Buffer, { 0 });
        commandBuffer.bindIndexBuffer(*Buffer, IndexOffset, IndexType);

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, *DescriptorSets[frameIndex], nullptr);
    }

    void UpdateUniformBuffer(uint32_t currentImage)
    {
        auto& swapChainExtent = GPipeline->GetSwapChainExtent();

        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        VKUniformBuffer ubo{};
        ubo.Model = rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.View = lookAt(glm::vec3(0.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.Proj = glm::perspective(glm::radians(60.0f), static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height), 0.1f, 100.0f);
        ubo.Model = glm::transpose(ubo.Model);
        ubo.View = glm::transpose(ubo.View);
        ubo.Proj = glm::transpose(ubo.Proj);
        ubo.Proj[1][1] *= -1;

        memcpy(*UniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }

    vk::raii::DescriptorSetLayout& GetDescriptorSetLayout()
    {
        return DescriptorSetLayout;
    }
private:
    vk::raii::Buffer Buffer = nullptr;
    vk::raii::DeviceMemory BufferMemory = nullptr;
    vk::DeviceSize IndexOffset = 0;
    vk::IndexType IndexType = vk::IndexType::eUint16;

    // Uniform buffer object
    VKUniformBuffer Ubo;

    std::vector<vk::raii::Buffer> UniformBuffers;
    std::vector<vk::raii::DeviceMemory> UniformBuffersMemory;
    std::vector<UniquePtr<void*>> UniformBuffersMapped;

    vk::raii::DescriptorPool DescriptorPool = nullptr;
    vk::raii::DescriptorSetLayout DescriptorSetLayout = nullptr;
    std::vector<vk::raii::DescriptorSet> DescriptorSets;

    void CreateVertexBuffer(const std::vector<VKVertex>& vertices)
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

    void CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& bufferMemory)
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

    uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
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

    void CopyBuffer(vk::raii::Buffer & srcBuffer, vk::raii::Buffer & dstBuffer, vk::DeviceSize size)
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

    // Uniform buffer object
    void CreateDescriptorSetLayout()
    {
        auto& device = GPipeline->GetDevice();

        vk::DescriptorSetLayoutBinding uboLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr);
        vk::DescriptorSetLayoutCreateInfo layoutInfo;
        layoutInfo.bindingCount = 1, layoutInfo.pBindings = &uboLayoutBinding;
        DescriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
    }

    void CreateUniformBuffers()
    {
        UniformBuffers.clear();
        UniformBuffersMemory.clear();
        UniformBuffersMapped.clear();

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vk::DeviceSize bufferSize = sizeof(Ubo);
            vk::raii::Buffer buffer({});
            vk::raii::DeviceMemory bufferMem({});
            CreateBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer, bufferMem);
            UniformBuffers.emplace_back(std::move(buffer));
            UniformBuffersMemory.emplace_back(std::move(bufferMem));
            UniformBuffersMapped.emplace_back(MakeUnique<void*>(UniformBuffersMemory[i].mapMemory(0, bufferSize)));
        }
    }

    void CreateDescriptorPool()
    {
        auto& device = GPipeline->GetDevice();

        vk::DescriptorPoolSize poolSize(vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT);
        vk::DescriptorPoolCreateInfo poolInfo;
        poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet; 
        poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;

        DescriptorPool = vk::raii::DescriptorPool(device, poolInfo);
    }

    void CreateDescriptorSets()
    {
        auto& device = GPipeline->GetDevice();

        std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *DescriptorSetLayout);
        vk::DescriptorSetAllocateInfo allocInfo;
        allocInfo.descriptorPool = DescriptorPool, allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size()), allocInfo.pSetLayouts = layouts.data();
        DescriptorSets.clear();
        DescriptorSets = device.allocateDescriptorSets(allocInfo);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vk::DescriptorBufferInfo bufferInfo;
            bufferInfo.buffer = UniformBuffers[i], bufferInfo.offset = 0, bufferInfo.range = sizeof(VKUniformBuffer);
            vk::WriteDescriptorSet descriptorWrite;
            descriptorWrite.dstSet = DescriptorSets[i], descriptorWrite.dstBinding = 0, descriptorWrite.dstArrayElement = 0, descriptorWrite.descriptorCount = 1, descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer, descriptorWrite.pBufferInfo = &bufferInfo;
            device.updateDescriptorSets(descriptorWrite, {});
        }
    }
};
