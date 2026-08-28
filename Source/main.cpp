#include "Runtime/RHI/RHI.hpp"
#include "Runtime/RHI/NRI/NRIShader.hpp"
#include "Runtime/RHI/NRI/NRIVertexBuffer.hpp"
#include "Runtime/RHI/NRI/NRITexture.hpp"
#include "Runtime/RHI/NRI/NRIMaterial.hpp"
#include "Runtime/RHI/NRI/NRIMesh.hpp"
#include <iostream>


int main(int argc, char** argv)
{
    const std::vector<NRIVertex> vertices =
    {
        NRIVertex {.Position = {-100.0f, 0.0f, 100.0f }, .TexCoord = {0.0f, 100.0f} },
        NRIVertex {.Position = { 100.0f, 0.0f, 100.0f }, .TexCoord = {100.0f, 100.0f} },
        NRIVertex {.Position = {-100.0f, 0.0f, -100.0f }, .TexCoord = {0.0f, 0.0f} },
        NRIVertex {.Position = { 100.0f, 0.0f, -100.0f }, .TexCoord = {100.0f, 0.0f} },
    };
    const std::vector<uint32_t> indices =
    {
        0, 1, 2,
        2, 1, 3,
    };

    auto rhi = MakeUnique<RHI>();
    GUserSettings->VSyncMode = VSYNC_TRIPLE_BUFFERED;
    GUserSettings->Anisotropic = ANISOTROPIC_8X;
    GUserSettings->MSAACount = MSAA_4X;
    if (!rhi->Init(nri::GraphicsAPI::D3D12, false, true))
        return -1;

    SharedPtr<NRIShader> vertexShader = MakeShared<NRIShader>(rhi->ICore, rhi->Device.Get());
    if (!vertexShader->Init(GetBaseDir().string() + "Resources/Shaders/Default.hlsl", SLANG_STAGE_VERTEX))
        return -1;

    SharedPtr<NRIShader> fragmentShader = MakeShared<NRIShader>(rhi->ICore, rhi->Device.Get());
    if (!fragmentShader->Init(GetBaseDir().string() + "Resources/Shaders/Default.hlsl", SLANG_STAGE_FRAGMENT))
        return -1;

    SharedPtr<NRITexture> texture = MakeShared<NRITexture>(rhi->ICore, rhi->IHelper, rhi->IStreamer, rhi->Streamer, rhi->Device.Get());
	if (!texture->Init(GetBaseDir().string() + "Resources/Textures/UVCheck.png", nri::AddressMode::REPEAT, nri::Filter::LINEAR, true))
        return -1;

	SharedPtr<NRIMaterial> material = MakeShared<NRIMaterial>(rhi->ICore, rhi->Device.Get(), rhi->SwapChainFormat, rhi->DepthFormat, vertexShader, fragmentShader);
    if (!material->Init(rhi->GCamera, { texture }))
        return -1;

    SharedPtr<NRIVertexBuffer> vertexBuffer = MakeShared<NRIVertexBuffer>(rhi->ICore, rhi->Device.Get());
    if (!vertexBuffer->Init(vertices, indices))
        return -1;

    auto mesh = MakeShared<NRIMesh>(vertexBuffer, material);

    rhi->GCamera->Position = glm::vec3(0.0f, 5.0f, 0.0f);

    bool shouldClose = false;
    GEvent->OnEvent = [&](SDL_Event& e)
    {
        switch (e.type)
        {
        case SDL_EVENT_QUIT:
            shouldClose = true;
            return;
        default:
            break;
        }
        rhi->UpdateSurface(e);
        GInput->UpdateEvent(e);
        rhi->GCamera->Inputs(e);
    };
    rhi->OnGraphicsSettingsChanged = [&]()
    {
        mesh->Material->UpdatePipeline();
    };
    rhi->OnBarrier = [&](nri::CommandBuffer* cmdBuf)
    {
        for (auto& texture : mesh->Material->Textures)
        {
            if (texture->NeedsMipmapGeneration())
                rhi->GMipmapPipeline->GenerateMipmaps(cmdBuf, texture);
            else
                texture->Barrier(cmdBuf);
        }
    };
    rhi->OnUploadBarrier = [&](nri::CommandBuffer* cmdBuf)
    {
        for (auto& texture : mesh->Material->Textures)
            texture->PrepareUploadBarrier(cmdBuf);
    };
    rhi->OnImgui = [&](nri::CommandBuffer* cmdBuf)
    {
        ImGui::Begin("Visualizador da Scene");
        ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);
        ImGui::End();
    };
    rhi->OnRender = [&](nri::CommandBuffer* cmdBuf)
    {
        mesh->Draw(rhi->ICore, cmdBuf);
    };
    while (!shouldClose)
    {
        rhi->Pool();
    }

    rhi->WaitIdle();

    mesh.reset();
    vertexBuffer.reset();
    material.reset();
    texture.reset();
    vertexShader.reset();
    fragmentShader.reset();

    rhi.reset();

    return 0;
}
