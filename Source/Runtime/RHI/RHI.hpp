#pragma once

#include "Config.hpp"
#include "Engine/Core/Object.hpp"
#include "Runtime/Utils/Logs.hpp"
#include "Runtime/Event.hpp"
#include "Runtime/Settings.hpp"
#include "Runtime/Tick.hpp"
#include "Runtime/RHI/NRI/NRIDevice.hpp"
#include <NRI.h>
#include <string>
#include <vector>
#include <Extensions/NRIDeviceCreation.h>
#include <Extensions/NRISwapChain.h>
#include <Extensions/NRIHelper.h>
#include <Extensions/NRIStreamer.h>
#include <SDL3/SDL.h>

#ifdef _WIN32
#include <Windows.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
#endif

struct SwapChainTexture
{
    nri::Fence* acquireSemaphore = nullptr;
    nri::Fence* releaseSemaphore = nullptr;
    nri::Texture* texture = nullptr;
    nri::Descriptor* colorAttachment = nullptr;
    nri::Texture* depthTexture = nullptr;
    nri::Descriptor* depthAttachment = nullptr;
    nri::Format attachmentFormat = nri::Format::UNKNOWN;
    nri::AccessLayoutStage colorState = {
        nri::AccessBits::NONE,
        nri::Layout::UNDEFINED,
        nri::StageBits::NONE
    };
    nri::AccessLayoutStage depthState = {
        nri::AccessBits::NONE,
        nri::Layout::UNDEFINED,
        nri::StageBits::NONE
    };
};

struct QueuedFrame
{
    nri::CommandAllocator* commandAllocator = nullptr;
    nri::CommandBuffer* commandBuffer = nullptr;
};

class RHI : public Object
{
public:
    nri::CoreInterface ICore = {};
    nri::HelperInterface IHelper = {};
    nri::StreamerInterface IStreamer = {};
    nri::SwapChainInterface ISwapChain = {};
    SDL_Window* Window = nullptr;
    NRIDevice Device;

    nri::SwapChain* SwapChain = nullptr;
    nri::Queue* GraphicsQueue = nullptr;
    nri::Fence* FrameFence = nullptr;
    nri::Color32f ClearColor = {0.0f, 1.0f, 0.0f, 1.0f}; // verde

    std::vector<QueuedFrame> QueuedFrames;
    std::vector<SwapChainTexture> SwapChainTextures;
    std::vector<nri::Memory*> SwapChainDepthMemory;

    nri::SwapChainDesc SwapChainDesc = {};
    uint64_t FrameIndex = 0;

    RHI() : Device(ICore)
    {
    }

    ~RHI()
    {
        Deinit();
    }

    bool Init(nri::GraphicsAPI graphicsAPI = nri::GraphicsAPI::NONE, bool useImgui = true,
              bool useValidationLayers = false)
    {
        nri::Result result = nri::Result::SUCCESS;

        if ((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) != 0)
        {
            Logs::Error("SDL video already initialized");
        }

        if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
        {
            Logs::SdlError();
            return false;
        }

        SDL_WindowFlags windowFlags = SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE;

        std::string platformName = SDL_GetPlatform();
        if (graphicsAPI == nri::GraphicsAPI::NONE)
        {
            graphicsAPI = (platformName == "Windows") ? nri::GraphicsAPI::D3D12 : nri::GraphicsAPI::VK;
        }

        if (graphicsAPI == nri::GraphicsAPI::VK)
            windowFlags |= SDL_WINDOW_VULKAN;

        Window = SDL_CreateWindow(ENGINE_NAME, 1024, 576, windowFlags);
        if (!Window)
        {
            Logs::SdlError();
            Deinit();
            return false;
        }

        result = Device.Init(graphicsAPI, useValidationLayers);
        if (result != nri::Result::SUCCESS)
        {
            Logs::RuntimeError("Failed to create NRI device: %d\nCode: %d", (int)graphicsAPI, (int)result);
            Deinit();
            return false;
        }

        // Interfaces
        result = nri::nriGetInterface(*Device.Get(), NRI_INTERFACE(nri::CoreInterface), &ICore);
        if (result != nri::Result::SUCCESS)
        {
            Logs::RuntimeError("Failed to get NRI core interface: %d", (int)result);
            Deinit();
            return false;
        }

        result = nri::nriGetInterface(*Device.Get(), NRI_INTERFACE(nri::StreamerInterface), &IStreamer);
        if (result != nri::Result::SUCCESS)
        {
            Logs::RuntimeError("Failed to get NRI streamer interface: %d", (int)result);
            Deinit();
            return false;
        }

        result = nri::nriGetInterface(*Device.Get(), NRI_INTERFACE(nri::HelperInterface), &IHelper);
        if (result != nri::Result::SUCCESS)
        {
            Logs::RuntimeError("Failed to get NRI helper interface: %d", (int)result);
            Deinit();
            return false;
        }

        result = nri::nriGetInterface(*Device.Get(), NRI_INTERFACE(nri::SwapChainInterface), &ISwapChain);
        if (result != nri::Result::SUCCESS)
        {
            Logs::RuntimeError("Failed to get NRI swap chain interface: %d", (int)result);
            Deinit();
            return false;
        }

        // Queue
        result = ICore.GetQueue(*Device.Get(), nri::QueueType::GRAPHICS, 0, GraphicsQueue);
        if (result != nri::Result::SUCCESS)
        {
            Logs::RuntimeError("Failed to get graphics queue: %d", (int)result);
            Deinit();
            return false;
        }

        // Frame fence (pacing CPU/GPU, timeline normal)
        result = ICore.CreateFence(*Device.Get(), 0, FrameFence);
        if (result != nri::Result::SUCCESS)
        {
            Logs::RuntimeError("Failed to create frame fence: %d", (int)result);
            Deinit();
            return false;
        }

        // SwapChain
        PopulateSwapChainDescBase(SwapChainDesc);

        if (!CreateSwapChainAndResources())
        {
            Deinit();
            return false;
        }

        // Queued frames (command allocator + buffer, one per fly)
        QueuedFrames.resize(GetQueuedFrameNum());
        for (QueuedFrame& queuedFrame : QueuedFrames)
        {
            result = ICore.CreateCommandAllocator(*GraphicsQueue, queuedFrame.commandAllocator);
            if (result != nri::Result::SUCCESS)
            {
                Logs::RuntimeError("Failed to create command allocator: %d", (int)result);
                Deinit();
                return false;
            }

            result = ICore.CreateCommandBuffer(*queuedFrame.commandAllocator, queuedFrame.commandBuffer);
            if (result != nri::Result::SUCCESS)
            {
                Logs::RuntimeError("Failed to create command buffer: %d", (int)result);
                Deinit();
                return false;
            }
        }

        return true;
    }

    void Deinit()
    {
        if (ICore.DeviceWaitIdle && Device.Get())
        {
            ICore.DeviceWaitIdle(Device.Get());
        }

        for (QueuedFrame& queuedFrame : QueuedFrames)
        {
            if (queuedFrame.commandBuffer) ICore.DestroyCommandBuffer(queuedFrame.commandBuffer);
            if (queuedFrame.commandAllocator) ICore.DestroyCommandAllocator(queuedFrame.commandAllocator);
        }
        QueuedFrames.clear();

        DestroySwapChainResources();

        if (SwapChain)
        {
            ISwapChain.DestroySwapChain(SwapChain);
            SwapChain = nullptr;
        }

        if (FrameFence)
        {
            ICore.DestroyFence(FrameFence);
            FrameFence = nullptr;
        }

        ISwapChain = {};
        IHelper = {};
        IStreamer = {};
        ICore = {};

        Device.Deinit();

        if (Window)
        {
            SDL_DestroyWindow(Window);
            Window = nullptr;
        }
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }

    void Pool()
    {
        GTick->UpdateCurrentTick();

        uint64_t frameTime = 0;
        /*
        if (GUserSettings->VSync == wgpu::PresentMode::Immediate && GUserSettings->GetFramerateLimit() > 0)
        {
            frameTime = 1000000000 / GUserSettings->GetFramerateLimit();
        }
        */
        GEvent->Run();
        Render();

        if (frameTime > 0 && frameTime > GTick->ElapsedNS())
        {
            SDL_DelayPrecise(frameTime - GTick->ElapsedNS());
        }

        GTick->UpdateDeltaTime();
    }

    void UpdateSurface(SDL_Event& e)
    {
        switch (e.type)
        {
        case SDL_EVENT_WINDOW_RESIZED:
            if (e.window.data1 > 0 && e.window.data2 > 0 &&
                (SwapChainDesc.width != e.window.data1 || SwapChainDesc.height != e.window.data2))
            {
                ResizeSwapChain((uint32_t)e.window.data1, (uint32_t)e.window.data2);
            }
            break;
        default:
            break;
        }
    }

private:
    uint32_t GetQueuedFrameNum() const
    {
        return (GUserSettings->VSyncMode == VSYNCTRIPLEBUFFERED) ? 3 : 2;
    }

    void PopulateSwapChainWindow(nri::SwapChainDesc& desc)
    {
        SDL_PropertiesID props = SDL_GetWindowProperties(Window);

#if defined(_WIN32)
        HWND hwnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        desc.window.windows.hwnd = hwnd;
#elif defined(__linux__)
        void* x11Display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
        uint64_t x11Window = (uint64_t)SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
        desc.window.x11.dpy = x11Display;
        desc.window.x11.window = x11Window;
#endif
    }

    void PopulateSwapChainDescBase(nri::SwapChainDesc& desc)
    {
        PopulateSwapChainWindow(desc);

        int outWidth, outHeight;
        SDL_GetWindowSize(Window, &outWidth, &outHeight);

        desc.queue = GraphicsQueue;
        desc.format = nri::SwapChainFormat::BT709_G22_8BIT;
        desc.width = (uint16_t)outWidth;
        desc.height = (uint16_t)outHeight;

        desc.flags = (GUserSettings->VSyncMode == VSYNCON ? nri::SwapChainBits::VSYNC : nri::SwapChainBits::NONE) |
            nri::SwapChainBits::ALLOW_TEARING;
        desc.textureNum = GUserSettings->VSyncMode == VSYNCTRIPLEBUFFERED ? 3 : 2;
        desc.queuedFrameNum = GUserSettings->VSyncMode == VSYNCTRIPLEBUFFERED ? 3 : 2;
    }

    bool CreateSwapChainAndResources()
    {
        nri::Result result = ISwapChain.CreateSwapChain(*Device.Get(), SwapChainDesc, SwapChain);
        if (result != nri::Result::SUCCESS)
        {
            Logs::RuntimeError("Failed to create NRI swap chain: %d", (int)result);
            return false;
        }

        uint32_t swapChainTextureNum = 0;
        nri::Texture* const* swapChainTextures = ISwapChain.GetSwapChainTextures(*SwapChain, swapChainTextureNum);

        nri::Format swapChainFormat = ICore.GetTextureDesc(*swapChainTextures[0]).format;
        nri::Format depthFormat = nri::Format::D32_SFLOAT_S8_UINT;

        SwapChainTextures.reserve(swapChainTextureNum);
        for (uint32_t i = 0; i < swapChainTextureNum; i++)
        {
            nri::TextureViewDesc textureViewDesc = {
                swapChainTextures[i], nri::TextureView::COLOR_ATTACHMENT, swapChainFormat
            };

            nri::Descriptor* colorAttachment = nullptr;
            result = ICore.CreateTextureView(textureViewDesc, colorAttachment);
            if (result != nri::Result::SUCCESS)
            {
                Logs::RuntimeError("Failed to create swap chain texture view %u: %d", i, (int)result);
                return false;
            }

            nri::Fence* acquireSemaphore = nullptr;
            result = ICore.CreateFence(*Device.Get(), nri::SWAPCHAIN_SEMAPHORE, acquireSemaphore);
            if (result != nri::Result::SUCCESS)
            {
                Logs::RuntimeError("Failed to create acquire semaphore %u: %d", i, (int)result);
                return false;
            }

            nri::Fence* releaseSemaphore = nullptr;
            result = ICore.CreateFence(*Device.Get(), nri::SWAPCHAIN_SEMAPHORE, releaseSemaphore);
            if (result != nri::Result::SUCCESS)
            {
                Logs::RuntimeError("Failed to create release semaphore %u: %d", i, (int)result);
                return false;
            }

            // Depth
            nri::TextureDesc depthTextureDesc = {};
            depthTextureDesc.type = nri::TextureType::TEXTURE_2D;
            depthTextureDesc.usage = nri::TextureUsageBits::DEPTH_STENCIL_ATTACHMENT;
            depthTextureDesc.format = depthFormat;
            depthTextureDesc.width = SwapChainDesc.width;
            depthTextureDesc.height = SwapChainDesc.height;
            depthTextureDesc.mipNum = 1;
            depthTextureDesc.layerNum = 1;
            depthTextureDesc.sampleNum = 1;
            depthTextureDesc.optimizedClearValue.depthStencil = {1.0f, 0};

            nri::Texture* depthTexture = nullptr;
            result = ICore.CreateTexture(*Device.Get(), depthTextureDesc, depthTexture);
            if (result != nri::Result::SUCCESS)
            {
                Logs::RuntimeError("Failed to create depth texture %u: %d", i, (int)result);
                return false;
            }

            nri::ResourceGroupDesc depthGroupDesc = {};
            depthGroupDesc.memoryLocation = nri::MemoryLocation::DEVICE;
            depthGroupDesc.textureNum = 1;
            depthGroupDesc.textures = &depthTexture;

            size_t memStart = SwapChainDepthMemory.size();
            uint32_t allocationNum = IHelper.CalculateAllocationNumber(*Device.Get(), depthGroupDesc);
            SwapChainDepthMemory.resize(memStart + allocationNum, nullptr);

            result = IHelper.AllocateAndBindMemory(*Device.Get(), depthGroupDesc,
                                                   SwapChainDepthMemory.data() + memStart);
            if (result != nri::Result::SUCCESS)
            {
                Logs::RuntimeError("Failed to allocate depth memory %u: %d", i, (int)result);
                return false;
            }

            nri::TextureViewDesc depthViewDesc = {};
            depthViewDesc.texture = depthTexture;
            depthViewDesc.type = nri::TextureView::DEPTH_STENCIL_ATTACHMENT;
            depthViewDesc.format = depthFormat;
            depthViewDesc.mipNum = 1;
            depthViewDesc.layerNum = 1;
            depthViewDesc.planes = nri::PlaneBits::DEPTH | nri::PlaneBits::STENCIL;

            nri::Descriptor* depthAttachment = nullptr;
            result = ICore.CreateTextureView(depthViewDesc, depthAttachment);
            if (result != nri::Result::SUCCESS)
            {
                Logs::RuntimeError("Failed to create depth view %u: %d", i, (int)result);
                return false;
            }

            SwapChainTexture& swapChainTexture = SwapChainTextures.emplace_back();
            swapChainTexture = {};
            swapChainTexture.acquireSemaphore = acquireSemaphore;
            swapChainTexture.releaseSemaphore = releaseSemaphore;
            swapChainTexture.texture = swapChainTextures[i];
            swapChainTexture.colorAttachment = colorAttachment;
            swapChainTexture.depthTexture = depthTexture;
            swapChainTexture.depthAttachment = depthAttachment;
            swapChainTexture.attachmentFormat = swapChainFormat;
        }

        return true;
    }

    void DestroySwapChainResources()
    {
        for (SwapChainTexture& swapChainTexture : SwapChainTextures)
        {
            // CORREÇÃO: Adicionado '*' para desreferenciar os ponteiros nas funções do NRI
            if (swapChainTexture.acquireSemaphore) ICore.DestroyFence(swapChainTexture.acquireSemaphore);
            if (swapChainTexture.releaseSemaphore) ICore.DestroyFence(swapChainTexture.releaseSemaphore);
            if (swapChainTexture.colorAttachment) ICore.DestroyDescriptor(swapChainTexture.colorAttachment);
            if (swapChainTexture.depthAttachment) ICore.DestroyDescriptor(swapChainTexture.depthAttachment);
            if (swapChainTexture.depthTexture) ICore.DestroyTexture(swapChainTexture.depthTexture);
        }
        SwapChainTextures.clear();

        for (nri::Memory* memory : SwapChainDepthMemory)
        {
            if (memory) ICore.FreeMemory(memory);
        }
        SwapChainDepthMemory.clear();
    }

    bool ResizeSwapChain(uint32_t width, uint32_t height)
    {
        if (ICore.DeviceWaitIdle)
        {
            ICore.DeviceWaitIdle(Device.Get());
        }

        DestroySwapChainResources();

        if (SwapChain)
        {
            ISwapChain.DestroySwapChain(SwapChain);
            SwapChain = nullptr;
        }

        SwapChainDesc.width = (uint16_t)width;
        SwapChainDesc.height = (uint16_t)height;
        PopulateSwapChainWindow(SwapChainDesc);

        if (!CreateSwapChainAndResources())
        {
            return false;
        }

        FrameIndex = 0;

        nri::Result result = ICore.CreateFence(*Device.Get(), 0, FrameFence);
        if (result != nri::Result::SUCCESS)
        {
            Logs::Error("Failed to recreate FrameFence during resize");
        }

        return true;
    }

    void Render()
    {
        uint32_t queuedFrameNum = GetQueuedFrameNum();
        uint32_t queuedFrameIndex = (uint32_t)(FrameIndex % queuedFrameNum);
        const QueuedFrame& queuedFrame = QueuedFrames[queuedFrameIndex];

        // Pacing: espera a GPU liberar este slot antes de resetar o allocator
        ICore.Wait(*FrameFence, FrameIndex >= queuedFrameNum ? 1 + FrameIndex - queuedFrameNum : 0);
        ICore.ResetCommandAllocator(*queuedFrame.commandAllocator);

        // Acquire
        uint32_t recycledSemaphoreIndex = (uint32_t)(FrameIndex % SwapChainTextures.size());
        nri::Fence* swapChainAcquireSemaphore = SwapChainTextures[recycledSemaphoreIndex].acquireSemaphore;
        uint32_t currentSwapChainTextureIndex = 0;
        nri::Result result = ISwapChain.AcquireNextTexture(
            *SwapChain,
            *swapChainAcquireSemaphore,
            currentSwapChainTextureIndex
        );

        if (result != nri::Result::SUCCESS)
            return;

        SwapChainTexture& swapChainTexture = SwapChainTextures[currentSwapChainTextureIndex];

        nri::CommandBuffer* commandBuffer = queuedFrame.commandBuffer;
        result = ICore.BeginCommandBuffer(
            *commandBuffer,
            nullptr
        );
        if (result != nri::Result::SUCCESS)
            return;

        nri::TextureBarrierDesc colorBarrier = {};
        colorBarrier.texture = swapChainTexture.texture;

        colorBarrier.before = swapChainTexture.colorState;
        colorBarrier.after = {
            nri::AccessBits::COLOR_ATTACHMENT_WRITE,
            nri::Layout::COLOR_ATTACHMENT,
            nri::StageBits::COLOR_ATTACHMENT
        };
        colorBarrier.mipNum = 1;
        colorBarrier.layerNum = 1;
        swapChainTexture.colorState = colorBarrier.after;

        nri::TextureBarrierDesc depthBarrier = {};
        depthBarrier.texture = swapChainTexture.depthTexture;
        depthBarrier.before = {
            nri::AccessBits::NONE,
            nri::Layout::UNDEFINED,
            nri::StageBits::NONE
        };
        depthBarrier.after = {
            nri::AccessBits::DEPTH_STENCIL_ATTACHMENT_WRITE,
            nri::Layout::DEPTH_STENCIL_ATTACHMENT,
            nri::StageBits::DEPTH_STENCIL_ATTACHMENT
        };
        depthBarrier.planes = nri::PlaneBits::DEPTH | nri::PlaneBits::STENCIL;
        depthBarrier.mipNum = 1;
        depthBarrier.layerNum = 1;

        nri::TextureBarrierDesc barriers[] = {
            colorBarrier,
            depthBarrier
        };
        nri::BarrierDesc barrierDesc = {};
        barrierDesc.textures = barriers;
        barrierDesc.textureNum = 2;

        ICore.CmdBarrier(
            *commandBuffer,
            barrierDesc
        );

        nri::AttachmentDesc colorAttachment = {};
        colorAttachment.descriptor = swapChainTexture.colorAttachment;
        colorAttachment.clearValue.color.f = ClearColor;
        colorAttachment.loadOp = nri::LoadOp::CLEAR;
        colorAttachment.storeOp = nri::StoreOp::STORE;

        nri::AttachmentDesc depthAttachment = {};
        depthAttachment.descriptor = swapChainTexture.depthAttachment;
        depthAttachment.clearValue.depthStencil = {1.0f, 0};
        depthAttachment.loadOp = nri::LoadOp::CLEAR;
        depthAttachment.storeOp = nri::StoreOp::DISCARD;

        nri::RenderingDesc renderingDesc = {};
        renderingDesc.colors = &colorAttachment;
        renderingDesc.colorNum = 1;
        renderingDesc.depth = depthAttachment;

        ICore.CmdBeginRendering(*commandBuffer, renderingDesc);
        {
            // Draw objects
            {
            }

            // Draw UI
            {
            }
        }
        ICore.CmdEndRendering(*commandBuffer);

        // Barrier: COLOR_ATTACHMENT -> PRESENT
        nri::TextureBarrierDesc presentBarrier = {};
        presentBarrier.texture = swapChainTexture.texture;
        presentBarrier.before = swapChainTexture.colorState;
        presentBarrier.after = {
            nri::AccessBits::NONE,
            nri::Layout::PRESENT,
            nri::StageBits::NONE
        };
        presentBarrier.mipNum = 1;
        presentBarrier.layerNum = 1;
        swapChainTexture.colorState = presentBarrier.after;

        nri::BarrierDesc presentBarrierDesc = {};
        presentBarrierDesc.textures = &presentBarrier;
        presentBarrierDesc.textureNum = 1;

        ICore.CmdBarrier(
            *commandBuffer,
            presentBarrierDesc
        );

        result = ICore.EndCommandBuffer(*commandBuffer);
        if (result != nri::Result::SUCCESS) return;

        // Submit
        nri::FenceSubmitDesc waitAcquire = {};
        waitAcquire.fence = swapChainAcquireSemaphore;
        waitAcquire.stages = nri::StageBits::COLOR_ATTACHMENT;

        nri::FenceSubmitDesc signalRelease = {};
        signalRelease.fence = swapChainTexture.releaseSemaphore;

        nri::QueueSubmitDesc submitDesc = {};
        submitDesc.waitFences = &waitAcquire;
        submitDesc.waitFenceNum = 1;
        submitDesc.commandBuffers = &commandBuffer;
        submitDesc.commandBufferNum = 1;
        submitDesc.signalFences = &signalRelease;
        submitDesc.signalFenceNum = 1;

        result = ICore.QueueSubmit(*GraphicsQueue, submitDesc);
        if (result != nri::Result::SUCCESS) return;

        // Present
        result = ISwapChain.QueuePresent(*SwapChain, *swapChainTexture.releaseSemaphore);
        if (result != nri::Result::SUCCESS) return;

        // Signal do frame fence depois do present (mesma ordem do Wrapper.cpp)
        nri::FenceSubmitDesc signalFrame = {};
        signalFrame.fence = FrameFence;
        signalFrame.value = 1 + FrameIndex;

        nri::QueueSubmitDesc frameFenceSubmitDesc = {};
        frameFenceSubmitDesc.signalFences = &signalFrame;
        frameFenceSubmitDesc.signalFenceNum = 1;
        ICore.QueueSubmit(*GraphicsQueue, frameFenceSubmitDesc);

        FrameIndex++;
    }
};
