#pragma once




#include "Vulkan/Pipeline.hpp"
//#include "Vulkan/Utils.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Core/Object.hpp"
#include "Utils/Logs.hpp"
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan.hpp>
#include <chrono>
//#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Texture.hpp"

struct VKVertex
{
    glm::vec3 Pos;
    glm::vec3 Color;
    glm::vec2 TexCoord;

    static vk::VertexInputBindingDescription GetBindingDescription()
    {
        vk::VertexInputBindingDescription d;
        d.binding = 0;
        d.stride = sizeof(VKVertex);
        d.inputRate =  vk::VertexInputRate::eVertex;
        return d;
    }
    static std::array<vk::VertexInputAttributeDescription, 3> GetAttributeDescriptions()
    {
        return {
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(VKVertex, Pos)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(VKVertex, Color)),
            vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(VKVertex, TexCoord))
        };
    }
};

struct VKUniformBuffer {
    glm::mat4 Model;
    glm::mat4 View;
    glm::mat4 Proj;
};

class VKVertexBuffer : public Object
{
public:
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

    std::vector<SharedPtr<VKTexture>> Textures;

    VKVertexBuffer() = default;
    ~VKVertexBuffer() override
    {
        Deinit();
    }

    template<typename T>
    void Init(const std::vector<VKVertex>& vertices, const std::vector<T>& indices, const std::vector<SharedPtr<VKTexture>>& textures = {})
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

        auto& device = GPipeline->Device;

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
        memoryAllocateInfoStaging.memoryTypeIndex = VKUtils::FindMemoryType(memRequirementsStaging.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        vk::raii::DeviceMemory stagingBufferMemory(device, memoryAllocateInfoStaging);

        stagingBuffer.bindMemory(stagingBufferMemory, 0);
        void* dataStaging = stagingBufferMemory.mapMemory(0, totalSize);
        memcpy(dataStaging, vertices.data(), vertexBufferSize);
        memcpy((char*)dataStaging + IndexOffset, indices.data(), indexBufferSize);
        stagingBufferMemory.unmapMemory();

        VKUtils::CreateBuffer(totalSize,
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            Buffer, BufferMemory);

        VKUtils::CopyBuffer(stagingBuffer, Buffer, totalSize);

        // Uniform buffer object
        CreateUniformBuffers();
        CreateDescriptorPool();
        Textures = textures;
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
        auto& commandBuffer = GPipeline->CommandBuffers[GPipeline->FrameIndex];

        commandBuffer.bindVertexBuffers(0, *Buffer, { 0 });
        commandBuffer.bindIndexBuffer(*Buffer, IndexOffset, IndexType);

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, *DescriptorSets[frameIndex], nullptr);
    }

    void UpdateUniformBuffer(uint32_t currentImage)
    {
        auto& swapChainExtent = GPipeline->SwapChainExtent;

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
private:

    void CreateVertexBuffer(const std::vector<VKVertex>& vertices)
    {
        auto& device = GPipeline->Device;
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
        VKUtils::CreateBuffer(bufferSize, vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, Buffer, BufferMemory);
        void* data = BufferMemory.mapMemory(0, bufferSize);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        BufferMemory.unmapMemory();
    }

    // Uniform buffer object
    void CreateDescriptorSetLayout()
    {
        auto& device = GPipeline->Device;

        std::array bindings = {
            vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr),
            vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr)
        };

        vk::DescriptorSetLayoutCreateInfo layoutInfo;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size()),
        layoutInfo.pBindings = bindings.data();
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
            VKUtils::CreateBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer, bufferMem);
            UniformBuffers.emplace_back(std::move(buffer));
            UniformBuffersMemory.emplace_back(std::move(bufferMem));
            UniformBuffersMapped.emplace_back(MakeUnique<void*>(UniformBuffersMemory[i].mapMemory(0, bufferSize)));
        }
    }

    void CreateDescriptorPool()
    {
        auto& device = GPipeline->Device;

        std::array poolSize{
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT),
            vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, MAX_FRAMES_IN_FLIGHT)
        };

        vk::DescriptorPoolCreateInfo poolInfo;
        poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
        poolInfo.poolSizeCount = (uint32_t)poolSize.size();
        poolInfo.pPoolSizes = poolSize.data();

        DescriptorPool = vk::raii::DescriptorPool(device, poolInfo);
    }

    void CreateDescriptorSets()
    {
        auto& device = GPipeline->Device;

        std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *DescriptorSetLayout);
        vk::DescriptorSetAllocateInfo allocInfo;
        allocInfo.descriptorPool = DescriptorPool, allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size()), allocInfo.pSetLayouts = layouts.data();
        DescriptorSets.clear();
        DescriptorSets = device.allocateDescriptorSets(allocInfo);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            std::vector<vk::WriteDescriptorSet> descriptorWrites;

            vk::DescriptorBufferInfo bufferInfo;
            bufferInfo.buffer = UniformBuffers[i], bufferInfo.offset = 0, bufferInfo.range = sizeof(VKUniformBuffer);
            vk::WriteDescriptorSet bufferWrite;
            bufferWrite.dstSet = DescriptorSets[i], bufferWrite.dstBinding = 0, bufferWrite.dstArrayElement = 0, bufferWrite.descriptorCount = 1,
                    bufferWrite.descriptorType = vk::DescriptorType::eUniformBuffer, bufferWrite.pBufferInfo = &bufferInfo;

            descriptorWrites.push_back(bufferWrite);

            for (auto& texture : Textures)
            {
                vk::DescriptorImageInfo imageInfo;
                imageInfo.sampler = *texture->TextureSampler, imageInfo.imageView = *texture->TextureImageView, imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                vk::WriteDescriptorSet imageWrite;
                imageWrite.dstSet = DescriptorSets[i], imageWrite.dstBinding = descriptorWrites.size(), imageWrite.dstArrayElement = 0, imageWrite.descriptorCount = 1,
                    imageWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler, imageWrite.pImageInfo = &imageInfo;
                descriptorWrites.push_back(imageWrite);
            }

            device.updateDescriptorSets(descriptorWrites, {});

            /*
            vk::DescriptorImageInfo imageInfo;
            imageInfo.sampler = TextureSampler,
            imageInfo.imageView = TextureImageView,
            imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

            vk::DescriptorBufferInfo bufferInfo;
            bufferInfo.buffer = UniformBuffers[i], bufferInfo.offset = 0, bufferInfo.range = sizeof(VKUniformBuffer);
            vk::WriteDescriptorSet uniformDescriptor;
            uniformDescriptor.dstSet = DescriptorSets[i], uniformDescriptor.dstBinding = 0, uniformDescriptor.dstArrayElement = 0, uniformDescriptor.descriptorCount = 1, uniformDescriptor.descriptorType = vk::DescriptorType::eUniformBuffer, uniformDescriptor.pBufferInfo = &bufferInfo;
            vk::WriteDescriptorSet imageDescriptor;
            imageDescriptor.dstSet = DescriptorSets[i], imageDescriptor.dstBinding = 1, imageDescriptor.dstArrayElement = 0, imageDescriptor.descriptorCount = 1, imageDescriptor.descriptorType = vk::DescriptorType::eCombinedImageSampler, imageDescriptor.pImageInfo = &imageInfo;
            
            std::array descriptorWrites{
                uniformDescriptor,
                imageDescriptor
            };
            device.updateDescriptorSets(descriptorWrites, {});
            */
        }
    }
};
