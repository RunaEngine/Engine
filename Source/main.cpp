#pragma once

#include "Engine/Engine.hpp"
#include "Runtime/RHI/RHI.hpp"
#include "Runtime/RHI/wgpu/WGMesh.hpp"
#include "Runtime/RHI/wgpu/WGMaterial.hpp"
#include "Runtime/RHI/wgpu/WGVertexBuffer.hpp"

int main(int argc, char** argv)
{
    const std::vector<WGVertex> vertices = 
    {
        WGVertex {.Position = {-0.5f, 0.5f, 0.0f }, .TexCoord = {0.0f, 1.0f} },
        WGVertex {.Position = { 0.5f, 0.5f, 0.0f }, .TexCoord = {1.0f, 1.0f} },
        WGVertex {.Position = {-0.5f, -0.5f, 0.0f }, .TexCoord = {0.0f, 0.0f} },
        WGVertex {.Position = { 0.5f, -0.5f, 0.0f }, .TexCoord = {1.0f, 0.0f} },

        WGVertex {.Position = {-0.5f, 0.5f, -0.5f }, .TexCoord = {0.0f, 1.0f} },
        WGVertex {.Position = { 0.5f, 0.5f, -0.5f }, .TexCoord = {1.0f, 1.0f} },
        WGVertex {.Position = {-0.5f, -0.5f, -0.5f }, .TexCoord = {0.0f, 0.0f} },
        WGVertex {.Position = { 0.5f, -0.5f, -0.5f }, .TexCoord = {1.0f, 0.0f} },
    };

    const std::vector<uint32_t> indices =
    {
        0, 1, 2,
        2, 1, 3,

        4, 5, 6,
        6, 5, 7,
    };

    auto currentDir = GetBaseDir();

    auto rhi = MakeUnique<RHI>();
    if (!rhi->Init(/*WGPUInstanceBackend_Vulkan*/))
        return -1;
    rhi->Multisample->Enabled = true;

    auto shader = MakeShared<WGShader>(rhi->Device->Get());
    if (!shader->Init(currentDir.string() + "Resources/Shaders/Default.wgsl"))
        return -1;

    auto texture = MakeShared<WGTexture>(rhi->Device->Get(), rhi->Queue);
    if (!texture->Init(currentDir.string() + "Resources/Textures/UVCheck.png"))
        return -1;

    auto material = MakeShared<WGMaterial>(rhi->Device->Get(), shader);
    material->Init(rhi->SurfaceConfig, rhi->GCamera->CameraBindGroupLayout, rhi->GCamera->CameraBindGroup, rhi->Multisample->Enabled, { texture });

    auto vertexBuffer = MakeShared<WGVertexBuffer>(rhi->Device->Get(), rhi->Queue);
    vertexBuffer->Init(vertices, indices);

    auto mesh = MakeShared<WGMesh>(vertexBuffer, material);
    //mesh->Init(material, vertexBuffer);

    rhi->GCamera->Position = glm::vec3(0.0f, 1.0f, 2.0f);

    bool shouldClose = false;
    rhi->GEvent->OnEvent = [&](SDL_Event& e)
        {
            rhi->UpdateSurface(e);
            switch (e.type)
            {
            case SDL_EVENT_QUIT:
                shouldClose = true;
                return;
            default:
                break;
            }
            rhi->GInput->UpdateEvent(e);
            rhi->GCamera->Inputs(e);
        };
    rhi->OnMsaaEnabledChange = [&](UniquePtr<WGMultisample>& multisample)
    {
        mesh->Material->UpdatePipeline();
    };
    rhi->OnRender = [&](WGPURenderPassEncoder pass, WGPUQueue queue)
        {
            mesh->Draw(pass);
        };
    while (!shouldClose)
    {
        rhi->Pool();
    }

    return 0;
}
