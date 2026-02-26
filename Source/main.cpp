#include <iostream>
#include "Engine/Engine.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/Shaders.h"
#include "Io/Event.h"
#include "Utils/System.h"
#include "Utils/Logs.h"

int main(int argc, char** argv)
{
    Event event;
    if (!GPipeline->Init())
        return -1;

    std::filesystem::path currentDir = BaseDir();

    std::filesystem::path slangfile = currentDir.string() + "Resources/Shaders/Slang.spv";
    VKShaders shader;
    if (!shader.Init(slangfile))
    {
        return -1;
    }


    bool shouldClose = false;
    event.OnEvent = [&](SDL_Event &e) {
        if (e.type == SDL_EVENT_QUIT)
        {
            shouldClose = true;
        }
    };
    GPipeline->OnRender = [&]()
    {
        auto& commandBuffer = GPipeline->GetCommandBuffer();
        auto& swapChainExtent = GPipeline->GetSwapChainExtent();

        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *shader.GetGraphicsPipeline());
        commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f));
        commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent));
        commandBuffer.draw(3, 1, 0, 0);
    };
    while (!shouldClose)
    {
        event.Run(EPool);
        GPipeline->Pool();
    }
    GPipeline->GetDevice().waitIdle();

    return 0;
}
