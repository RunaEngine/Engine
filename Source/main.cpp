#include "Engine/Engine.hpp"
#include "Vulkan/Pipeline.hpp"
#include "Vulkan/VertexBuffer.hpp"
#include "Vulkan/Shader.hpp"
#include "Io/Event.hpp"
#include "Utils/System.hpp"
#include <SDL3/SDL.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <filesystem>
#include <iostream>
#include <vector>
#include <cstdint>
#include <chrono>

int main(int argc, char** argv)
{
    Event event;
    if (!GPipeline->Init())
        return -1;

    auto currentDir = GetBaseDir();

    const std::vector<VKVertex> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
    };

    const std::vector<uint16_t> indices = {
        0, 1, 2, 
        2, 3, 0
    };

    auto texturefile = currentDir.string() + "Resources/Textures/Planks.png";
    SharedPtr<VKTexture> texture = MakeShared<VKTexture>();
    if (!texture->Init(texturefile))
    {
        return -1;
    }

    VKVertexBuffer vbo;
    vbo.Init(vertices, indices, {texture});

    auto slangfile = currentDir.string() + "Resources/Shaders/Slang.spv";
    auto bindingDescription = VKVertex::GetBindingDescription();
    VKShader shader;
    if ( !shader.Init(slangfile, bindingDescription, VKVertex::GetAttributeDescriptions(), vbo.DescriptorSetLayout) )
    {
        return -1;
    }

    bool shouldClose = false;
    event.OnEvent = [&](SDL_Event& e)
    {
        if (e.type == SDL_EVENT_QUIT)
        {
            shouldClose = true;
        }
        GInput->UpdateEvent(e);
    };
    GPipeline->OnSwap = [&](uint32_t frameIndex)
    {
        vbo.UpdateUniformBuffer(frameIndex);
    };
    GPipeline->OnRender = [&](uint32_t frameIndex)
    {
        auto& commandBuffer = GPipeline->CommandBuffers[GPipeline->FrameIndex];
        auto& swapChainExtent = GPipeline->SwapChainExtent;
        shader.Bind();

        commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f));
        commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent));

        vbo.Bind(frameIndex, shader.PipelineLayout);
        commandBuffer.drawIndexed(indices.size(), 1, 0, 0, 0);
    };
    while (!shouldClose)
    {
        event.Run(EPool);
        GPipeline->Pool();
    }
    GPipeline->Device.waitIdle();

    return 0;
}
