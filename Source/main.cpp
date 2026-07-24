#include "Engine/Engine.hpp"
/*
#include "Runtime/Vulkan/Pipeline.hpp"
#include "Runtime/Vulkan/VertexBuffer.hpp"
#include "Runtime/Vulkan/Shader.hpp"
#include "Runtime/Event.hpp"
#include "Runtime/Utils/System.hpp"
#include "Runtime/Io/Import.hpp"
#include <SDL3/SDL.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <filesystem>
#include <vector>
#include <chrono>
*/
#include "Runtime/RHI/RHI.hpp"
#include "Runtime/RHI/wgpu/WGMesh.hpp"
#include "Runtime/RHI/wgpu/WGMaterial.hpp"
#include "Runtime/RHI/wgpu/WGVertexBuffer.hpp"

int main(int argc, char** argv)
{
    /*
    GPipeline->GUserSettings->Vsync = TripleBuffering;
    GPipeline->GUserSettings->MSAASamples = MSAA8X;
    //GPipeline->GUserSettings->SetFramerateLimit(60);

    Event event;

    if (!GPipeline->Init())
        return -1;

    auto currentDir = GetBaseDir();

    const std::vector<VKVertex> vertices = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}
    };

    const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
    };


    //Import assimp;

    auto texturefile = currentDir.string() + "Resources/Textures/UVCheck.png";
    SharedPtr<VKTexture> texture = MakeShared<VKTexture>();
    if (!texture->Init(texturefile))
    {
        return -1;
    }

    VKVertexBuffer vbo;
    //auto gltfFile = currentDir.string() + "Resources/Cube/cube.glb";
    //assimp.LoadAsset(gltfFile, vbo);
    //vbo.UpdateTextures({texture});
    vbo.Init(vertices, indices, {texture});

    auto slangfile = currentDir.string() + "Resources/Shaders/Slang.spv";
    auto bindingDescription = VKVertex::GetBindingDescription();
    VKShader shader;
    if ( !shader.Init(slangfile, bindingDescription, VKVertex::GetAttributeDescriptions(), vbo.DescriptorSetLayout) )
    {
        return -1;
    }

    bool shouldClose = false;
    GPipeline->GEvent->OnEvent = [&](SDL_Event& e)
    {
        if (e.type == SDL_EVENT_QUIT)
        {
            shouldClose = true;
        }
        GPipeline->GInput->UpdateEvent(e);
        GPipeline->GCamera->Inputs(e);
    };
    GPipeline->OnSwap = [&](uint32_t frameIndex)
    {
        vbo.UpdateUniformBuffer(frameIndex);
    };
    GPipeline->OnRender = [&](uint32_t frameIndex)
    {
        auto& commandBuffer = GPipeline->Render->CommandBuffers[GPipeline->Render->FrameIndex];
        auto& swapChainExtent = GPipeline->Render->SwapChainExtent;
        shader.Bind();

        commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f));
        commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent));

        vbo.Bind(frameIndex, shader.PipelineLayout);
        commandBuffer.drawIndexed(vbo.IndexSize, 1, 0, 0, 0);
    };
    while (!shouldClose)
    {
        GPipeline->Pool();
    }
    GPipeline->LogicalDevice->Get().waitIdle();
    */
    const std::vector<WGVertex> vertices = {
        WGVertex { .Position = {0.5f, 0.5f, 0.0f }, .TexCoord = {1.0f, 1.0f} },
        WGVertex { .Position = {-0.5f, 0.5f, 0.0f }, .TexCoord = {0.0f, 1.0f} },
        WGVertex { .Position = {-0.5f, -0.5f, 0.0f }, .TexCoord = {0.0f, 0.0f} },
        WGVertex { .Position = { 0.5f, -0.5f, 0.0f }, .TexCoord = {1.0f, 0.0f} },
    };

    const std::vector<uint32_t> indices =
    {
        0, 1, 2,
        2, 3, 0
    };

    auto currentDir = GetBaseDir();

    auto rhi = MakeUnique<RHI>();
    if (!rhi->Init(/*WGPUInstanceBackend_DX12*/))
        return -1;

    auto shader = MakeShared<WGShader>(rhi->Device.Get());
    if (!shader->Init(currentDir.string() + "Resources/Shaders/Default.wgsl"))
        return -1;

    auto texture = MakeShared<WGTexture>(rhi->Device.Get(), rhi->Queue);
    if (!texture->Init(currentDir.string() + "Resources/Textures/UVCheck.png"))
        return -1;

    auto material = MakeShared<WGMaterial>(rhi->Device.Get(), shader);
    material->Init(rhi->SurfaceConfig, { texture });

    auto vertexBuffer = MakeShared<WGVertexBuffer>(rhi->Device.Get(), rhi->Queue);
    vertexBuffer->Init(vertices, indices);

    auto mesh = MakeShared<WGMesh>(vertexBuffer, material);
    //mesh->Init(material, vertexBuffer);

    rhi->OnRender = [&](WGPURenderPassEncoder pass, WGPUQueue queue)
    {
        mesh->Draw(pass);
    };
    rhi->Pool();

    return 0;
}
