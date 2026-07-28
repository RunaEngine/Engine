#pragma once

#include "Config.hpp"
#include "Engine/Core/Object.hpp"
#include "Runtime/Event.hpp"
#include "Runtime/Input.hpp"
#include "Runtime/Tick.hpp"
#include "Runtime/Utils/Logs.hpp"
#include "Runtime/RHI/wgpu/WGCamera.hpp"
#include "Runtime/RHI/wgpu/WGMultisample.hpp"
#include "Runtime/RHI/wgpu/WGDepthBuffer.hpp"
#include "Runtime/RHI/wgpu/WGPipeline.hpp"
#include <sdl3webgpu.h>
#include <dawn/webgpu_cpp.h>
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
    wgpu::Surface Surface;
    wgpu::Instance Instance;
    wgpu::Adapter Adapter;
    wgpu::Device Device;
    wgpu::TextureFormat SurfaceFormat = {};
    wgpu::SurfaceConfiguration SurfaceConfig = {};
    UniquePtr<WGMultisample> Multisample = nullptr;
    UniquePtr<WGDepthBuffer> DepthBuffer = nullptr;
    wgpu::Queue Queue;

    std::function<void(UniquePtr<WGMultisample>&)> OnMsaaEnabledChange;
    std::function<void(wgpu::RenderPassEncoder&, wgpu::Queue&)> OnRender;

    RHI()
    {
        Multisample = MakeUnique<WGMultisample>();
        DepthBuffer = MakeUnique<WGDepthBuffer>(Multisample->Enabled);
    }

    ~RHI()
    {
        Deinit();
    }

    bool Init(wgpu::BackendType backend = wgpu::BackendType::Null)
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

        SDL_WindowFlags windowFlags = SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE;
        if (backend == wgpu::BackendType::Vulkan)
            windowFlags |= SDL_WINDOW_VULKAN;
        Window = SDL_CreateWindow(
            ENGINE_NAME,
            1024, 576,
            windowFlags
        );
        if (!Window)
        {
            Logs::SdlError();
            Deinit();
            return false;
        }

        // Instance
        wgpu::InstanceFeatureName featureNames[] = {
            wgpu::InstanceFeatureName::TimedWaitAny
        };
        wgpu::InstanceDescriptor instanceDescriptor = {
            .requiredFeatureCount = sizeof(featureNames) / sizeof(featureNames[0]),
            .requiredFeatures = featureNames,
        };
        Instance = wgpu::CreateInstance(&instanceDescriptor);

        // Surface
        Surface = wgpu::Surface::Acquire(SDL_GetWGPUSurface(Instance.Get(), Window));

        // Adapter
        wgpu::RequestAdapterOptions adapterOptions = {};
        std::string_view os = SDL_GetPlatform();
        if (backend == wgpu::BackendType::Null || backend == wgpu::BackendType::Undefined)
        {
            if (os == "Windows")
            {
                adapterOptions.backendType = wgpu::BackendType::D3D12;
            }
            else if (os == "Linux" || os == "Android")
            {
                adapterOptions.backendType = wgpu::BackendType::Vulkan;
            }
            else
            {
                adapterOptions.backendType = wgpu::BackendType::Metal;
            }
        }
        else
        {
            adapterOptions.backendType = backend;
        }
        auto adapterFuture = Instance.RequestAdapter(
            &adapterOptions,
            wgpu::CallbackMode::WaitAnyOnly,
            [](wgpu::RequestAdapterStatus status,
               wgpu::Adapter adapter,
               const char* message,
               RHI* userdata)
            {
                if (status == wgpu::RequestAdapterStatus::Success)
                {
                    userdata->Adapter = adapter;
                }
                else
                {
                    Logs::Error("Failed to get adapter: %s\n", message);
                }
            },
            this
        );

        wgpu::FutureWaitInfo adapterWaitInfo = {.future = adapterFuture};
        wgpu::WaitStatus adapterWaitStatus = Instance.WaitAny(1, &adapterWaitInfo, UINT64_MAX);
        if (adapterWaitStatus != wgpu::WaitStatus::Success)
        {
            Logs::Error("Failed to get adapter");
            return false;
        }

        wgpu::AdapterInfo adapterInfo = {};
        Adapter.GetInfo(&adapterInfo);
        Logs::Log("Adapter: %.*s", (int)adapterInfo.device.length, adapterInfo.device.data);

        // Device
        wgpu::FeatureName requiredFeatures[] = {
            wgpu::FeatureName::Depth32FloatStencil8,
        };
        wgpu::DeviceDescriptor deviceDesc = {};
        deviceDesc.requiredFeatures = requiredFeatures;
        deviceDesc.requiredFeatureCount = sizeof(requiredFeatures) / sizeof(requiredFeatures[0]);

        deviceDesc.SetUncapturedErrorCallback(
            [](const wgpu::Device&, wgpu::ErrorType errorType, wgpu::StringView message)
            {
                Logs::Error("Uncaptured device error (%d): %.*s\n",
                            (int)errorType, (int)message.length, message.data);
            }
        );
        deviceDesc.SetDeviceLostCallback(
            wgpu::CallbackMode::AllowSpontaneous,
            [](const wgpu::Device&, wgpu::DeviceLostReason reason, wgpu::StringView message)
            {
                Logs::Error("Device lost (reason %d): %.*s\n",
                            (int)reason, (int)message.length, message.data);
            }
        );

        auto deviceFuture = Adapter.RequestDevice(
            &deviceDesc,
            wgpu::CallbackMode::WaitAnyOnly,
            [](wgpu::RequestDeviceStatus status,
               wgpu::Device receivedDevice,
               const char* message,
               RHI* userdata)
            {
                if (status == wgpu::RequestDeviceStatus::Success)
                {
                    userdata->Device = receivedDevice;
                }
                else
                {
                    Logs::Error("Failed to get device: %s\n", message);
                }
            },
            this
        );

        wgpu::FutureWaitInfo deviceWaitInfo = {.future = deviceFuture};
        wgpu::WaitStatus deviceWaitStatus = Instance.WaitAny(1, &deviceWaitInfo, UINT64_MAX);
        if (deviceWaitStatus != wgpu::WaitStatus::Success)
        {
            Logs::Error("Failed to get device");
            return false;
        }

        // Queue
        Queue = Device.GetQueue();

        // Surface
        wgpu::SurfaceCapabilities capabilities = {};
        wgpu::ConvertibleStatus status = Surface.GetCapabilities(Adapter, &capabilities);

        if (status.status != wgpu::Status::Success)
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
        ///*
                for (size_t i = 0; i < capabilities.presentModeCount; i++)
                {
                    Logs::Log("Present mode: %d", capabilities.presentModes[i]);
                }
        //*/
        wgpu::PresentMode presentMode = wgpu::PresentMode::Fifo;

        int windowWidth = 1024;
        int windowHeight = 576;
        SDL_GetWindowSize(Window, &windowWidth, &windowHeight);

        SurfaceConfig.usage = wgpu::TextureUsage::RenderAttachment;
        SurfaceConfig.format = SurfaceFormat;
        SurfaceConfig.device = Device;
        SurfaceConfig.width = windowWidth;
        SurfaceConfig.height = windowHeight;
        SurfaceConfig.presentMode = presentMode;
        SurfaceConfig.alphaMode = wgpu::CompositeAlphaMode::Auto;

        Surface.Configure(&SurfaceConfig);

        //wgpuSurfaceCapabilitiesFreeMembers(capabilities);

        GCamera = MakeShared<WGCamera>(Device, Queue, Window, GInput);
        Multisample->Init(Device, SurfaceConfig);
        DepthBuffer->Init(Device, SurfaceConfig);

        return true;
    }

    void Deinit()
    {
        if (Window) SDL_DestroyWindow(Window);
        if (SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)
        {
            SDL_Quit();
        }
    }

    void Pool()
    {
        GTick->UpdateCurrentTick();
        if (Multisample->Enabled != Multisample->PreviousEnabled)
        {
            Multisample->PreviousEnabled = Multisample->Enabled;
            Multisample->Init(Device, SurfaceConfig);
            DepthBuffer->Init(Device, SurfaceConfig);
            if (OnMsaaEnabledChange)
                OnMsaaEnabledChange(Multisample);
        }
        GEvent->Run();
        GCamera->Tick(GTick->Delta());
        GCamera->UpdateMatrix();
        Render();
        GTick->UpdateDeltaTime();
    }

    void UpdateSurface(SDL_Event& e)
    {
        switch (e.type)
        {
        case SDL_EVENT_WINDOW_RESIZED:
            if (e.window.data1 > 0 && e.window.data2 > 0 &&
                (SurfaceConfig.width != e.window.data1 || SurfaceConfig.height != e.window.data2))
            {
                SurfaceConfig.width = e.window.data1;
                SurfaceConfig.height = e.window.data2;
                ConfigureSurface();
                Multisample->Init(Device, SurfaceConfig);
                DepthBuffer->Init(Device, SurfaceConfig);
            }
            break;
        default:
            break;
        }
    }

private:
    void ConfigureSurface()
    {
        if (!Surface.Get() || !Device.Get())
        {
            Logs::Error("Cannot configure surface: invalid surface or device");
            return;
        }

        if (SurfaceConfig.width == 0 || SurfaceConfig.height == 0)
        {
            Logs::Log("Skipping surface configure because width or height is zero");
            return;
        }

        Surface.Configure(&SurfaceConfig);
    }

    void Render()
    {
        wgpu::SurfaceTexture output = {};
        Surface.GetCurrentTexture(&output);

        switch (output.status)
        {
        case wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal:
        case wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal:
            break;
        case wgpu::SurfaceGetCurrentTextureStatus::Timeout:
        case wgpu::SurfaceGetCurrentTextureStatus::Error:
            Logs::Log("Surface error");
            return;
        case wgpu::SurfaceGetCurrentTextureStatus::Outdated:
            Logs::Log("Surface outdated");
            ConfigureSurface();
            return;
        case wgpu::SurfaceGetCurrentTextureStatus::Lost:
            Logs::RuntimeError("Device lost!");
        default:
            return;
        }

        wgpu::TextureView view = output.texture.CreateView();

        wgpu::CommandEncoderDescriptor encoderDesc = {};
        wgpu::CommandEncoder encoder = Device.CreateCommandEncoder();

        wgpu::RenderPassColorAttachment colorAttachment = {
            .view = Multisample->Enabled ? Multisample->TextureView : view,
            .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
            .resolveTarget = Multisample->Enabled ? view : nullptr,
            .loadOp = wgpu::LoadOp::Clear,
            .storeOp = Multisample->Enabled ? wgpu::StoreOp::Discard : wgpu::StoreOp::Store,
            .clearValue = {
                .r = 0.1,
                .g = 0.2,
                .b = 0.3,
                .a = 1.0,
            }
        };

        wgpu::RenderPassDepthStencilAttachment depthAtt = {
            .view = DepthBuffer->DepthTextureView,
            .depthLoadOp = wgpu::LoadOp::Clear,
            .depthStoreOp = wgpu::StoreOp::Store,
            .depthClearValue = 1.0f,
            .stencilLoadOp = wgpu::LoadOp::Clear,
            .stencilStoreOp = wgpu::StoreOp::Discard,
            .stencilClearValue = 0
        };
        wgpu::RenderPassDescriptor renderDesc = {
            .colorAttachmentCount = 1,
            .colorAttachments = &colorAttachment,
            .depthStencilAttachment = &depthAtt,
            .occlusionQuerySet = nullptr,
            .timestampWrites = nullptr,
        };

        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderDesc);

        // Draw call
        if (OnRender)
            OnRender(pass, Queue);

        pass.End();

        wgpu::CommandBuffer cmdBuffer = encoder.Finish();
        Queue.Submit(1, &cmdBuffer);
        Surface.Present();
    }
};
