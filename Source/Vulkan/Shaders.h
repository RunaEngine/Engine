#pragma once

#include "Engine/Core/Object.h"
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <filesystem>

class VKShaders : public Object
{
public:
    VKShaders() = default;
    ~VKShaders() override;

    bool Init(const std::filesystem::path& filepath);
    void Deinit();

    vk::raii::Pipeline& GetGraphicsPipeline();
private:
    vk::raii::PipelineLayout PipelineLayout = nullptr;
    vk::raii::Pipeline GraphicsPipeline = nullptr;
    std::vector<vk::raii::ShaderModule> ShaderModules;

    void CreateGraphicsPipeline(const std::vector<vk::PipelineShaderStageCreateInfo>& shaderStages);
    vk::raii::ShaderModule CreateShaderModule(const uint32_t* data, size_t size);
};
