#pragma once

#include "Config.hpp"
#include "Engine/Core/Object.hpp"
#include "Runtime/Event.hpp"
#include "Runtime/Input.hpp"
#include "Runtime/Tick.hpp"
#include "Runtime/Utils/Logs.hpp"
#include "Runtime/RHI/wgpu/WGCamera.hpp"
#include "Runtime/RHI/wgpu/WGAdapter.hpp"
#include "Runtime/RHI/wgpu/WGDevice.hpp"
#include "Runtime/RHI/wgpu/WGPipeline.hpp"
#include <sdl3webgpu.h>
#include <webgpu/wgpu.h>
#include <SDL3/SDL.h>


class RHI : public Object
{
private:

public:
    // Globals
    SharedPtr<WGCamera> GCamera = nullptr;
    SharedPtr<Event> GEvent = MakeShared<Event>();
    SharedPtr<Input> GInput = MakeShared<Input>();
    SharedPtr<Tick> GTick = MakeShared<Tick>();

    SDL_Window* Window = nullptr;
    WGPUSurface Surface = nullptr;
    WGPUInstance Instance = nullptr;
    WGAdapter Adapter;
    WGDevice Device;
    WGPUTextureFormat SurfaceFormat = {};
    WGPUSurfaceConfigurationExtras SurfaceExtras = {};
    WGPUSurfaceConfiguration SurfaceConfig = {};
    WGPUQueue Queue = nullptr;

    std::function<void(SDL_Event&)> OnEvent;
    std::function<void(WGPURenderPassEncoder, WGPUQueue)> OnRender;

    RHI() = default;

    ~RHI()
    {
        Deinit();
    }

    bool Init(WGPUInstanceBackend backend = WGPUInstanceBackend_Primary)
    {
        if (SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)
        {
            Logs::Error("SDL video already initialized");
            return false;
        }

        if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
        {
            Logs::SdlError();
            return false;
        }

        Window = SDL_CreateWindow(
            ENGINE_NAME,
            1024, 576,
            SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE
        );
        if (!Window)
        {
            Logs::SdlError();
            Deinit();
            return false;
        }

        // Instance
        std::string_view os = SDL_GetPlatform();

        WGPUInstanceExtras extras = {};
        extras.chain.sType = (WGPUSType)WGPUSType_InstanceExtras;
        extras.chain.next = nullptr;
        if ((backend == WGPUInstanceBackend_Primary || backend == WGPUInstanceBackend_All) && os == "Windows")
        {
            extras.backends = WGPUInstanceBackend_DX12;
        }
        else
        {
            extras.backends = backend;
        }


        WGPUInstanceDescriptor desc = {};
        desc.nextInChain = &extras.chain;

        Instance = wgpuCreateInstance(&desc);
        if (!Instance)
        {
            Logs::Error("Failed to init wgpu instance");
            Deinit();
            return false;
        }

        // Surface
        Surface = SDL_GetWGPUSurface(Instance, Window);
        if (!Surface)
        {
            Logs::RuntimeError("Failed to create wgpu surface");
            Deinit();
            return false;
        }

        // Adapter
        if (!Adapter.Init(Instance, Surface))
        {
            Deinit();
            return false;
        }
        WGPUAdapterInfo adapterInfo = {};

        wgpuAdapterGetInfo(Adapter.Get(), &adapterInfo);
        Logs::Log("Adapter: %.*s", (int)adapterInfo.device.length, adapterInfo.device.data);

        // Device
        if (!Device.Init(Instance, Adapter.Get()))
        {
            Deinit();
            return false;
        }

        // Queue
        Queue = wgpuDeviceGetQueue(Device.Get());
        if (!Queue)
        {
            Logs::Error("Failed to get queue");
            Deinit();
            return false;
        }

        // Surface
        WGPUSurfaceCapabilities capabilities = {};

        WGPUStatus status = wgpuSurfaceGetCapabilities(
            Surface,
            Adapter.Get(),
            &capabilities
        );

        if (status != WGPUStatus_Success)
        {
            Logs::Error("Failed to get surface capabilities");
            return false;
        }

        if (capabilities.formatCount > 0)
        {
            SurfaceFormat = capabilities.formats[0];
        }
        else
        {
            Logs::Error("No surface formats available");
            return false;
        }
        /*
                for (size_t i = 0; i < capabilities.presentModeCount; i++)
                {
                    Logs::Log("Present mode: %d", capabilities.presentModes[i]);
                }
        */
        WGPUPresentMode presentMode = WGPUPresentMode_Immediate;
        SurfaceExtras.chain.sType = (WGPUSType)WGPUSType_SurfaceConfigurationExtras;
        SurfaceExtras.chain.next = nullptr;
        SurfaceExtras.desiredMaximumFrameLatency = 2;

        int windowWidth = 1024;
        int windowHeight = 576;
        SDL_GetWindowSize(Window, &windowWidth, &windowHeight);

        SurfaceConfig.nextInChain = (WGPUChainedStruct*)&SurfaceExtras;
        SurfaceConfig.usage = WGPUTextureUsage_RenderAttachment;
        SurfaceConfig.format = SurfaceFormat;
        SurfaceConfig.device = Device.Get();
        SurfaceConfig.width = windowWidth;
        SurfaceConfig.height = windowHeight;
        SurfaceConfig.presentMode = presentMode;
        SurfaceConfig.alphaMode = WGPUCompositeAlphaMode_Auto;

        wgpuSurfaceConfigure(
            Surface,
            &SurfaceConfig
        );

        wgpuSurfaceCapabilitiesFreeMembers(capabilities);

        GCamera = MakeShared<WGCamera>(Device.Get(), Queue, Window, GInput);

        return true;
    }

    void Deinit()
    {
        if (Queue) wgpuQueueRelease(Queue);
        Device.Deinit();
        Adapter.Deinit();
        if (Surface) wgpuSurfaceRelease(Surface);
        if (Instance) wgpuInstanceRelease(Instance);
        if (Window) SDL_DestroyWindow(Window);
        if (SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)
        {
            SDL_Quit();
        }
    }

    void Pool()
    {
        GTick->UpdateCurrentTick();
        GEvent->Run(EPool);
        GCamera->Tick(GTick->Delta());
        GCamera->UpdateMatrix();
        Render();
        GTick->UpdateDeltaTime();
    }

    void ConfigureSurface()
    {
        if (!Surface || !Device.Get())
        {
            Logs::Error("Cannot configure surface: invalid surface or device");
            return;
        }

        if (SurfaceConfig.width == 0 || SurfaceConfig.height == 0)
        {
            Logs::Log("Skipping surface configure because width or height is zero");
            return;
        }

        wgpuSurfaceConfigure(
            Surface,
            &SurfaceConfig
        );
    }

private:
    

    void Render()
    {
        WGPUSurfaceTexture output = {};

        wgpuSurfaceGetCurrentTexture(
            Surface,
            &output
        );

        switch (output.status)
        {
        case WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal:
        case WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal:
            break;
        case WGPUSurfaceGetCurrentTextureStatus_Timeout:
        case WGPUSurfaceGetCurrentTextureStatus_Error:
            Logs::Log("Surface error");
            return;
        case WGPUSurfaceGetCurrentTextureStatus_Outdated:
            Logs::Log("Surface outdated");
            ConfigureSurface();
            return;
        case WGPUSurfaceGetCurrentTextureStatus_Lost:
            Logs::RuntimeError("Device lost!");
        default:
            return;
        }

        WGPUTextureView view = wgpuTextureCreateView(
            output.texture,
            nullptr
        );

        if (!view)
        {
            Logs::Error("Texture view is null");
            return;
        }

        WGPUCommandEncoderDescriptor encoderDesc = {};
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(
            Device.Get(),
            &encoderDesc
        );

        WGPURenderPassDescriptor renderDesc = {};

        WGPURenderPassColorAttachment colorAttachment = {};
        colorAttachment.view = view;
        colorAttachment.resolveTarget = nullptr;
        colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        colorAttachment.loadOp = WGPULoadOp_Clear;
        colorAttachment.storeOp = WGPUStoreOp_Store;
        colorAttachment.clearValue.r = 0.1;
        colorAttachment.clearValue.g = 0.2;
        colorAttachment.clearValue.b = 0.3;
        colorAttachment.clearValue.a = 1.0;

        renderDesc.colorAttachments = &colorAttachment;
        renderDesc.colorAttachmentCount = 1;
        renderDesc.depthStencilAttachment = nullptr;
        renderDesc.occlusionQuerySet = nullptr;
        renderDesc.timestampWrites = nullptr;

        WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &renderDesc);

        // Draw call
        if (OnRender)
            OnRender(pass, Queue);

        wgpuRenderPassEncoderEnd(pass);

        auto cmdBuffer = wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuQueueSubmit(Queue, 1, &cmdBuffer);
        wgpuSurfacePresent(Surface);

        wgpuTextureViewRelease(view);
        wgpuRenderPassEncoderRelease(pass);
        wgpuCommandEncoderRelease(encoder);
        wgpuCommandBufferRelease(cmdBuffer);
        wgpuTextureRelease(output.texture);
    }
};
