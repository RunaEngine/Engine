#pragma once

#include "Config.hpp"
#include "Engine/Core/Object.hpp"
#include "Runtime/Utils/Logs.hpp"
#include "Runtime/Event.hpp"
#include "Runtime/Settings.hpp"
#include "Runtime/Tick.hpp"
#include "Runtime/RHI/NRI/NRIDevice.hpp"
#include "Runtime/RHI/NRI/NRICamera.hpp"
#include "Runtime/RHI/NRI/NRIColorTexture.hpp"
#include "Runtime/RHI/NRI/NRIDepthTexture.hpp"
#include "Runtime/RHI/NRI/NRIMultisampleTexture.hpp"
#include "Runtime/RHI/NRI/NRIMipmap.hpp"
#include "Runtime/RHI/NRI/NRImGui.hpp"
#include <NRI.h>
#include <string>
#include <vector>
#include <Extensions/NRISwapChain.h>
#include <Extensions/NRIHelper.h>
#include <Extensions/NRIStreamer.h>
#include <SDL3/SDL.h>

#ifdef _WIN32
#include <Windows.h>
#elif defined(__linux__)
#include <wayland-client-core.h>
#ifdef None
#undef None
#endif
#ifdef Bool
#undef Bool
#endif
#endif

struct SwapChainTexture
{
    nri::Fence* acquireSemaphore = nullptr;
    nri::Fence* releaseSemaphore = nullptr;
    NRIColorTexture colorTexture;
    NRIDepthTexture depthTexture;
    NRIMultisampleTexture msaaTexture;
};

struct QueuedFrame
{
    nri::CommandAllocator* commandAllocator = nullptr;
    nri::CommandBuffer* commandBuffer = nullptr;
};

class RHI : public Object
{
public:
    SharedPtr<NRICamera> GCamera = nullptr;
    UniquePtr<NRIMipmap> GMipmapPipeline = nullptr;
    UniquePtr<NRImGui> GImGui = nullptr;

    nri::CoreInterface ICore = {};
    nri::HelperInterface IHelper = {};
    nri::StreamerInterface IStreamer = {};
    nri::SwapChainInterface ISwapChain = {};
    nri::Streamer* Streamer = nullptr;
    SDL_Window* Window = nullptr;
    NRIDevice Device;

    nri::SwapChain* SwapChain = nullptr;
    nri::Queue* GraphicsQueue = nullptr;
    nri::Fence* FrameFence = nullptr;
    nri::Color32f ClearColor = { 0.0f, 1.0f, 0.0f, 1.0f };

    std::vector<QueuedFrame> QueuedFrames;
    std::vector<SwapChainTexture> SwapChainTextures;

    nri::SwapChainDesc SwapChainDesc = {};
    nri::Format SwapChainFormat = nri::Format::UNKNOWN;
    nri::Format DepthFormat = nri::Format::UNKNOWN;
    uint64_t FrameIndex = 0;

    // Important for pipeline event trigger
    EMSAACount PreviousMSAACount = MSAA_DISABLED;
    EAnisotropic PreviousAnisotropic = ANISOTROPIC_DISABLED;

    std::function<void(nri::CommandBuffer* commandBuffer)> OnBarrier;
    std::function<void()> OnGraphicsSettingsChanged;
    std::function<void(nri::CommandBuffer* commandBuffer)> OnRender;
    std::function<void(nri::CommandBuffer* commandBuffer)> OnImgui;

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
        {
            windowFlags |= SDL_WINDOW_VULKAN;
            SDL_Vulkan_LoadLibrary(nullptr);
        }

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

        // Streamer
        nri::StreamerDesc streamerDesc = {};
        streamerDesc.dynamicBufferMemoryLocation = nri::MemoryLocation::HOST_UPLOAD;
        streamerDesc.dynamicBufferDesc = {
            .size = 0,
            .structureStride = 0,
            .usage = nri::BufferUsageBits::VERTEX_BUFFER | nri::BufferUsageBits::INDEX_BUFFER
        };
        streamerDesc.constantBufferMemoryLocation = nri::MemoryLocation::HOST_UPLOAD;
        streamerDesc.constantBufferSize = 1 * 1024 * 1024;
        streamerDesc.queuedFrameNum = GetQueuedFrameNum();
        IStreamer.CreateStreamer(*Device.Get(), streamerDesc, Streamer);

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

        // Globals
        SharedPtr<NRIShader> mipmapShader = MakeShared<NRIShader>(ICore, Device.Get());
        if (!mipmapShader->Init("Resources/Shaders/Mipmap.hlsl", SLANG_STAGE_COMPUTE))
        {
            Logs::Error("Failed to load mipmap compute shader");
            Deinit();
            return false;
        }
        GMipmapPipeline = MakeUnique<NRIMipmap>(ICore, Device.Get());
        if (!GMipmapPipeline->Init(mipmapShader))
        {
            Deinit();
            return false;
        }

        GCamera = MakeShared<NRICamera>(ICore, Device.Get(), Window);
        if (!GCamera->Init())
        {
            Deinit();
            return false;
        }

        GImGui = MakeUnique<NRImGui>(ICore, Device.Get());
        if (useImgui)
        {
            if (!GImGui->Init(Window, SwapChainFormat, GetQueuedFrameNum(), GraphicsQueue))
            {
                Logs::Error("Failed to initialize engine debug interface.");
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

        GMipmapPipeline->Deinit();
        GCamera->Deinit();
        if (GImGui)
        {
            GImGui->Deinit();
            GImGui = nullptr;
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

        if (Streamer)
        {
            IStreamer.DestroyStreamer(Streamer);
            Streamer = nullptr;
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
        if (GUserSettings->VSyncMode == VSYNC_OFF && GUserSettings->GetFramerateLimit() > 0)
        {
            frameTime = 1000000000 / GUserSettings->GetFramerateLimit();
        }

        if (GUserSettings->MSAACount != PreviousMSAACount || GUserSettings->Anisotropic != PreviousAnisotropic)
        {
            ResizeSwapChain(SwapChainDesc.width, SwapChainDesc.height);
            if (OnGraphicsSettingsChanged) OnGraphicsSettingsChanged();
            PreviousMSAACount = GUserSettings->MSAACount;
            PreviousAnisotropic = GUserSettings->Anisotropic;
        }

        GEvent->Run(GImGui->IsInitialized());
        GCamera->Tick(GTick->Delta());
        GCamera->UpdateMatrix();
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
        return (GUserSettings->VSyncMode == VSYNC_TRIPLE_BUFFERED) ? 3 : 2;
    }

    void PopulateSwapChainWindow(nri::SwapChainDesc& desc)
    {
        SDL_PropertiesID props = SDL_GetWindowProperties(Window);

#if defined(_WIN32)
        HWND hwnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        desc.window.windows.hwnd = hwnd;
#elif defined(__linux__)
        void* waylandDisplay = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
        void* waylandSurface = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
        desc.window.wayland.display = waylandDisplay;
        desc.window.wayland.surface = waylandSurface;
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

        desc.flags = (GUserSettings->VSyncMode == VSYNC_ON ? nri::SwapChainBits::VSYNC : nri::SwapChainBits::NONE) |
            nri::SwapChainBits::ALLOW_TEARING;
        desc.textureNum = GUserSettings->VSyncMode == VSYNC_TRIPLE_BUFFERED ? 3 : 2;
        desc.queuedFrameNum = GUserSettings->VSyncMode == VSYNC_TRIPLE_BUFFERED ? 3 : 2;
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

        SwapChainFormat = ICore.GetTextureDesc(*swapChainTextures[0]).format;

        SwapChainTextures.reserve(swapChainTextureNum);
        for (uint32_t i = 0; i < swapChainTextureNum; i++)
        {
            SwapChainTexture& swapChainTexture = SwapChainTextures.emplace_back();
            swapChainTexture = {};

            if (!swapChainTexture.colorTexture.InitFromExisting(ICore, swapChainTextures[i], SwapChainFormat))
                return false;

            if (!swapChainTexture.depthTexture.Create(ICore, IHelper, *Device.Get(), SwapChainDesc.width,
                SwapChainDesc.height))
                return false;

            DepthFormat = swapChainTexture.depthTexture.Format;

            if (GUserSettings->MSAACount != MSAA_DISABLED)
            {
                uint8_t samples = static_cast<uint8_t>(GUserSettings->MSAACount);
                if (!swapChainTexture.msaaTexture.Create(ICore, IHelper, *Device.Get(), SwapChainDesc.width,
                    SwapChainDesc.height, SwapChainFormat, samples))
                    return false;
            }

            ICore.CreateFence(*Device.Get(), nri::SWAPCHAIN_SEMAPHORE, swapChainTexture.acquireSemaphore);
            ICore.CreateFence(*Device.Get(), nri::SWAPCHAIN_SEMAPHORE, swapChainTexture.releaseSemaphore);
        }

        return true;
    }

    void DestroySwapChainResources()
    {
        for (SwapChainTexture& swapChainTexture : SwapChainTextures)
        {
            if (swapChainTexture.acquireSemaphore) ICore.DestroyFence(swapChainTexture.acquireSemaphore);
            if (swapChainTexture.releaseSemaphore) ICore.DestroyFence(swapChainTexture.releaseSemaphore);

            swapChainTexture.colorTexture.Deinit(ICore);
            swapChainTexture.depthTexture.Deinit(ICore);
            swapChainTexture.msaaTexture.Deinit(ICore);
        }
        SwapChainTextures.clear();
    }

    bool ResizeSwapChain(uint32_t width, uint32_t height)
    {
        if (ICore.DeviceWaitIdle) ICore.DeviceWaitIdle(Device.Get());
        DestroySwapChainResources();
        if (SwapChain) { ISwapChain.DestroySwapChain(SwapChain); SwapChain = nullptr; }

        SwapChainDesc.width = (uint16_t)width;
        SwapChainDesc.height = (uint16_t)height;
        PopulateSwapChainWindow(SwapChainDesc);

        if (!CreateSwapChainAndResources()) return false;

        FrameIndex = 0;

        // FIX: destruir o fence antigo antes de recriar
        if (FrameFence)
        {
            ICore.DestroyFence(FrameFence);
            FrameFence = nullptr;
        }

        nri::Result result = ICore.CreateFence(*Device.Get(), 0, FrameFence);
        if (result != nri::Result::SUCCESS)
            Logs::Error("Failed to recreate FrameFence during resize");

        return true;
    }

    void Render()
    {
        uint32_t queuedFrameNum = GetQueuedFrameNum();
        uint32_t queuedFrameIndex = (uint32_t)(FrameIndex % queuedFrameNum);
        const QueuedFrame& queuedFrame = QueuedFrames[queuedFrameIndex];

        // Pacing: wait for the GPU to release this slot before resetting the allocator
        ICore.Wait(*FrameFence, FrameIndex >= queuedFrameNum ? 1 + FrameIndex - queuedFrameNum : 0);
        ICore.ResetCommandAllocator(*queuedFrame.commandAllocator);

        // Acquire
        uint32_t recycledSemaphoreIndex = (uint32_t)(FrameIndex % SwapChainTextures.size());
        nri::Fence* swapChainAcquireSemaphore = SwapChainTextures[recycledSemaphoreIndex].acquireSemaphore;
        uint32_t currentSwapChainTextureIndex = 0;
        nri::Result result = ISwapChain.AcquireNextTexture(*SwapChain, *swapChainAcquireSemaphore, currentSwapChainTextureIndex);

        if (result != nri::Result::SUCCESS) return;

        SwapChainTexture& swapChainTexture = SwapChainTextures[currentSwapChainTextureIndex];
        bool useMSAA = (GUserSettings->MSAACount != MSAA_DISABLED);

        nri::CommandBuffer* commandBuffer = queuedFrame.commandBuffer;
        if (ICore.BeginCommandBuffer(*commandBuffer, nullptr) != nri::Result::SUCCESS) return;

        IStreamer.CmdCopyStreamedData(*commandBuffer, *Streamer);
        if (OnBarrier) OnBarrier(commandBuffer);

        // Step 1: Initial barriers configuration
        std::vector<nri::TextureBarrierDesc> initialBarriers;

        // Configure barrier for the Main Color Target
        nri::TextureBarrierDesc colorBarrier = {};
        colorBarrier.texture = swapChainTexture.colorTexture.Texture;

        // present cycles. LoadOp::CLEAR makes this safe.
        colorBarrier.before = nri::AccessLayoutStage{
            .access = nri::AccessBits::NONE,
            .layout = nri::Layout::UNDEFINED,
            .stages = nri::StageBits::NONE
        };
        colorBarrier.mipNum = 1;
        colorBarrier.layerNum = 1;

        if (useMSAA)
        {
            // If MSAA is enabled, the SwapChain will only receive the copy from the resolve at the end
            colorBarrier.after = {
                .access = nri::AccessBits::RESOLVE_DESTINATION,
                .layout = nri::Layout::RESOLVE_DESTINATION,
                .stages = nri::StageBits::RESOLVE
            };
            initialBarriers.push_back(colorBarrier);

            // Configure the barrier for the MSAA texture that will receive the rendering
            nri::TextureBarrierDesc msaaBarrier = {};
            msaaBarrier.texture = swapChainTexture.msaaTexture.Texture;
            msaaBarrier.before = swapChainTexture.msaaTexture.CurrentState;
            msaaBarrier.after = {
                .access = nri::AccessBits::COLOR_ATTACHMENT_WRITE,
                .layout = nri::Layout::COLOR_ATTACHMENT,
                .stages = nri::StageBits::COLOR_ATTACHMENT
            };
            msaaBarrier.mipNum = 1;
            msaaBarrier.layerNum = 1;
            swapChainTexture.msaaTexture.CurrentState = msaaBarrier.after;
            initialBarriers.push_back(msaaBarrier);
        }
        else
        {
            // Normal flow: Render directly to the SwapChain
            colorBarrier.after = {
                .access = nri::AccessBits::COLOR_ATTACHMENT_WRITE,
                .layout = nri::Layout::COLOR_ATTACHMENT,
                .stages = nri::StageBits::COLOR_ATTACHMENT
            };
            initialBarriers.push_back(colorBarrier);
        }
        swapChainTexture.colorTexture.CurrentState = colorBarrier.after;

        // Configure Depth barrier
        nri::TextureBarrierDesc depthBarrier = {};
        depthBarrier.texture = swapChainTexture.depthTexture.Texture;
        depthBarrier.before = swapChainTexture.depthTexture.CurrentState;
        depthBarrier.after = {
            .access = nri::AccessBits::DEPTH_STENCIL_ATTACHMENT_WRITE,
            .layout = nri::Layout::DEPTH_STENCIL_ATTACHMENT,
            .stages = nri::StageBits::DEPTH_STENCIL_ATTACHMENT
        };
        depthBarrier.planes = nri::PlaneBits::DEPTH | nri::PlaneBits::STENCIL;
        depthBarrier.mipNum = 1;
        depthBarrier.layerNum = 1;
        initialBarriers.push_back(depthBarrier);

        nri::BarrierDesc barrierDesc = {};
        barrierDesc.textures = initialBarriers.data();
        barrierDesc.textureNum = (uint32_t)initialBarriers.size();
        ICore.CmdBarrier(*commandBuffer, barrierDesc);

        // Step 2: Render Pass
        nri::AttachmentDesc colorAttachment = {};
        colorAttachment.descriptor = useMSAA ? swapChainTexture.msaaTexture.ColorAttachment : swapChainTexture.colorTexture.ColorAttachment;
        colorAttachment.clearValue.color.f = ClearColor;
        colorAttachment.loadOp = nri::LoadOp::CLEAR;
        colorAttachment.storeOp = nri::StoreOp::STORE;

        nri::AttachmentDesc depthAttachment = {};
        depthAttachment.descriptor = swapChainTexture.depthTexture.DepthAttachment;
        depthAttachment.clearValue.depthStencil = { 1.0f, 0 };
        depthAttachment.loadOp = nri::LoadOp::CLEAR;
        depthAttachment.storeOp = nri::StoreOp::DISCARD;

        nri::RenderingDesc renderingDesc = {};
        renderingDesc.colors = &colorAttachment;
        renderingDesc.colorNum = 1;
        renderingDesc.depth = depthAttachment;

        ICore.CmdBeginRendering(*commandBuffer, renderingDesc);
        nri::Viewport viewport = {
            .x = 0.0f,
            .y = 0.0f, .width = static_cast<float>(SwapChainDesc.width),
            .height = static_cast<float>(SwapChainDesc.height),
            .depthMin = 0.0f,
            .depthMax = 1.0f
        };
        ICore.CmdSetViewports(*commandBuffer, &viewport, 1);

        nri::Rect scissor = {
            .x = 0,
            .y = 0,
            .width = static_cast<uint32_t>(SwapChainDesc.width),
            .height = static_cast<uint32_t>(SwapChainDesc.height)
        };
        ICore.CmdSetScissors(*commandBuffer, &scissor, 1);

        if (OnRender) OnRender(commandBuffer);

        if (GImGui->IsInitialized())
        {
            GImGui->BeginFrame();
            if (OnImgui) OnImgui(commandBuffer);
            GImGui->EndAndRender(commandBuffer, colorAttachment.descriptor, SwapChainDesc.width, SwapChainDesc.height);
        }
        ICore.CmdEndRendering(*commandBuffer);

        // Step 3: MSAA Resolve (if enabled)
        if (useMSAA)
        {
            nri::TextureBarrierDesc msaaResolveBarrier = {};
            msaaResolveBarrier.texture = swapChainTexture.msaaTexture.Texture;
            msaaResolveBarrier.before = swapChainTexture.msaaTexture.CurrentState;
            msaaResolveBarrier.after = {
                nri::AccessBits::RESOLVE_SOURCE, nri::Layout::RESOLVE_SOURCE, nri::StageBits::RESOLVE
            };
            msaaResolveBarrier.mipNum = 1;
            msaaResolveBarrier.layerNum = 1;
            swapChainTexture.msaaTexture.CurrentState = msaaResolveBarrier.after;

            nri::BarrierDesc resolveBarrierGroup = {};
            resolveBarrierGroup.textures = &msaaResolveBarrier;
            resolveBarrierGroup.textureNum = 1;
            ICore.CmdBarrier(*commandBuffer, resolveBarrierGroup);

            ICore.CmdResolveTexture(*commandBuffer, *swapChainTexture.colorTexture.Texture, nullptr,
                *swapChainTexture.msaaTexture.Texture, nullptr, nri::ResolveOp::AVERAGE);
        }

        // Step 4: Preparation for present
        nri::TextureBarrierDesc presentBarrier = {};
        presentBarrier.texture = swapChainTexture.colorTexture.Texture;
        presentBarrier.before = swapChainTexture.colorTexture.CurrentState;

        presentBarrier.after = {
            .access = nri::AccessBits::NONE,
            .layout = nri::Layout::PRESENT,
            .stages = nri::StageBits::NONE
        };
        presentBarrier.mipNum = 1;
        presentBarrier.layerNum = 1;
        swapChainTexture.colorTexture.CurrentState = presentBarrier.after;
        nri::BarrierDesc presentBarrierDesc = {};
        presentBarrierDesc.textures = &presentBarrier;
        presentBarrierDesc.textureNum = 1;
        ICore.CmdBarrier(*commandBuffer, presentBarrierDesc);
        if (ICore.EndCommandBuffer(*commandBuffer) != nri::Result::SUCCESS) return;

        // Submit
        nri::FenceSubmitDesc waitAcquire = {};
        waitAcquire.fence = swapChainAcquireSemaphore;
        waitAcquire.stages = useMSAA ? nri::StageBits::RESOLVE : nri::StageBits::COLOR_ATTACHMENT;
        waitAcquire.value = 0;
        nri::FenceSubmitDesc signalRelease = {};
        signalRelease.fence = swapChainTexture.releaseSemaphore;
        signalRelease.stages = nri::StageBits::NONE;
        signalRelease.value = 0;
        nri::QueueSubmitDesc submitDesc = {};
        submitDesc.waitFences = &waitAcquire;
        submitDesc.waitFenceNum = 1;
        submitDesc.commandBuffers = &commandBuffer;
        submitDesc.commandBufferNum = 1;
        submitDesc.signalFences = &signalRelease;
        submitDesc.signalFenceNum = 1;
        if (ICore.QueueSubmit(*GraphicsQueue, submitDesc) != nri::Result::SUCCESS) return;
        ISwapChain.QueuePresent(*SwapChain, *swapChainTexture.releaseSemaphore);

        if (GImGui->IsInitialized())
        {
            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }
        }

        // FrameFence tracking
        nri::FenceSubmitDesc signalFrame = {};
        signalFrame.fence = FrameFence;
        signalFrame.stages = nri::StageBits::NONE;
        signalFrame.value = 1 + FrameIndex;
        nri::QueueSubmitDesc frameFenceSubmitDesc = {};
        frameFenceSubmitDesc.signalFences = &signalFrame;
        frameFenceSubmitDesc.signalFenceNum = 1;
        ICore.QueueSubmit(*GraphicsQueue, frameFenceSubmitDesc);
        IStreamer.EndStreamerFrame(*Streamer);
        FrameIndex++;
    }
};
