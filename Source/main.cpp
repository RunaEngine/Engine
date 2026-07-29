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
        WGVertex {.Position = {-100.0f, 0.0f, 100.0f }, .TexCoord = {0.0f, 100.0f} },
        WGVertex {.Position = { 100.0f, 0.0f, 100.0f }, .TexCoord = {100.0f, 100.0f} },
        WGVertex {.Position = {-100.0f, 0.0f, -100.0f }, .TexCoord = {0.0f, 0.0f} },
        WGVertex {.Position = { 100.0f, 0.0f, -100.0f }, .TexCoord = {100.0f, 0.0f} },
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
    GUserSettings->bMSAAEnabled = true;
    GUserSettings->Anisotropic = e16X;
    GUserSettings->VSync = wgpu::PresentMode::Fifo;

    auto shader = MakeShared<WGShader>(rhi->Device);
    if (!shader->Init(currentDir.string() + "Resources/Shaders/Default.wgsl"))
        return -1;

    auto texture = MakeShared<WGTexture>(rhi->Device, rhi->Queue);
    if (!texture->Init(currentDir.string() + "Resources/Textures/UVCheck.png", wgpu::AddressMode::Repeat, wgpu::FilterMode::Linear, true))
        return -1;

    auto material = MakeShared<WGMaterial>(rhi->Device, rhi->Queue, rhi->SurfaceConfig, shader);
    material->Init(rhi->GCamera->CameraBindGroupLayout, rhi->GCamera->CameraBindGroup, { texture });

    auto vertexBuffer = MakeShared<WGVertexBuffer>(rhi->Device, rhi->Queue);
    vertexBuffer->Init(vertices, indices);

    auto mesh = MakeShared<WGMesh>(vertexBuffer, material);
    //mesh->Init(material, vertexBuffer);

    rhi->GCamera->Position = glm::vec3(0.0f, -10.0f, 0.0f);

    bool shouldClose = false;
    GEvent->OnEvent = [&](SDL_Event& e)
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
        GInput->UpdateEvent(e);
        rhi->GCamera->Inputs(e);
    };
    rhi->OnMsaaEnabledChange = [&](UniquePtr<WGMultisample>& multisample)
    {
        mesh->Material->UpdatePipeline();
    };
    rhi->OnAnisatropicChange = [&]()
    {
        mesh->Material->UpdatePipeline();
    };
    rhi->OnImguiRender = [&](ImGuiIO& io)
    {
        ImGui::Begin("SDL3 + Dawn");
        ImGui::Text("Rendered via WebGPU and SDL3!");
        ImGui::End();
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
