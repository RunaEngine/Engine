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
        WGVertex {.Position = {-500.0f, 0.0f, 500.0f }, .TexCoord = {0.0f, 100.0f} },
        WGVertex {.Position = { 500.0f, 0.0f, 500.0f }, .TexCoord = {100.0f, 100.0f} },
        WGVertex {.Position = {-500.0f, 0.0f, -500.0f }, .TexCoord = {0.0f, 0.0f} },
        WGVertex {.Position = { 500.0f, 0.0f, -500.0f }, .TexCoord = {100.0f, 0.0f} },
    };

    const std::vector<uint32_t> indices =
    {
        0, 1, 2,
        2, 1, 3,
    };

    auto currentDir = GetBaseDir();

    auto rhi = MakeUnique<RHI>();
    if (!rhi->Init(/*wgpu::BackendType::Vulkan*/))
        return -1;
    rhi->Multisample->Enabled = false;

    auto shader = MakeShared<WGShader>(rhi->Device);
    if (!shader->Init(currentDir.string() + "Resources/Shaders/Default.wgsl"))
        return -1;

    auto texture = MakeShared<WGTexture>(rhi->Device, rhi->Queue);
    if (!texture->Init(currentDir.string() + "Resources/Textures/UVCheck.png"))
        return -1;

    auto material = MakeShared<WGMaterial>(rhi->Device, rhi->Queue, shader);
    material->Init(rhi->SurfaceConfig, rhi->GCamera->CameraBindGroupLayout, rhi->GCamera->CameraBindGroup, rhi->Multisample->Enabled, { texture }, true);

    auto vertexBuffer = MakeShared<WGVertexBuffer>(rhi->Device, rhi->Queue);
    vertexBuffer->Init(vertices, indices);

    auto mesh = MakeShared<WGMesh>(vertexBuffer, material);
    //mesh->Init(material, vertexBuffer);

    rhi->GCamera->Position = glm::vec3(0.0f, -10.0f, 0.0f);

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
    rhi->OnRender = [&](wgpu::RenderPassEncoder& pass, wgpu::Queue& queue)
        {
            mesh->Draw(pass);
        };
    while (!shouldClose)
    {
        rhi->Pool();
    }

    return 0;
}
