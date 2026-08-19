#include "Runtime/RHI/RHI.hpp"
#include <iostream>

#include "Runtime/RHI/NRI/NRIShader.hpp"
#include "Runtime/RHI/NRI/NRIVertex.hpp"

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
    GUserSettings->VSyncMode = VSYNCTRIPLEBUFFERED;
    if (!rhi->Init(nri::GraphicsAPI::D3D12, false, false))
        return -1;

    NRIVertexBuffer vertexBuffer(rhi->ICore, rhi->Device.Get());
    if (!vertexBuffer.Init(vertices, indices))
        return -1;

    NRIShader shader(rhi->ICore, rhi->Device.Get());
    shader.Init(GetBaseDir().string() + "Resources/Shaders/Default.vs.hlsl");

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
        rhi->UpdateSurface(e);
    };
    while (!shouldClose)
    {
        rhi->Pool();
    }

    return 0;
}
