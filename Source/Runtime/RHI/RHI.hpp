#pragma once

#include "Config.hpp"
#include "Engine/Core/Object.hpp"
#include "Runtime/Event.hpp"
#include "Runtime/Input.hpp"
#include "Runtime/Tick.hpp"
#include "Runtime/Utils/Logs.hpp"
#include "Runtime/RHI/Utils.hpp"
#include "Runtime/RHI/wgpu/WGCamera.hpp"
#include "Runtime/RHI/wgpu/WGMultisample.hpp"
#include "Runtime/RHI/wgpu/WGDepthBuffer.hpp"
#include "Runtime/RHI/wgpu/WGPipeline.hpp"
#include "Runtime/RHI/wgpu/WGImgui.hpp"
#include <dawn/webgpu_cpp.h>
#include <SDL3/SDL.h>
#include <set>

class GameUserSettings;

class RHI : public Object
{
public:
    // Globals
    SharedPtr<WGCamera> GCamera = nullptr;

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

    UniquePtr<WGImgui> Imgui = nullptr;

    // Important for pipeline event trigger
    bool PreviousMSAAEnabled = false;
    EAnisotropic PreviousAnisotropic = eDisabled;

    std::function<void(UniquePtr<WGMultisample>&)> OnMsaaEnabledChange;
    std::function<void()> OnAnisatropicChange;
    std::function<void(ImGuiIO&)> OnImguiRender;
    std::function<void(wgpu::RenderPassEncoder&, wgpu::Queue&)> OnRender;

    RHI()
    {
        Multisample = MakeUnique<WGMultisample>();
        DepthBuffer = MakeUnique<WGDepthBuffer>();
    }

    ~RHI()
    {
        Deinit();
    }

    bool Init(wgpu::BackendType backend = wgpu::BackendType::Null, bool useImgui = true)
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
        Surface = WGPUtils::GetWGPUSurfaceFromSDL3(Instance, Window);
        Logs::Error("Surface ptr: %p", (void*)Surface.Get());

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
        wgpu::Future adapterFuture = Instance.RequestAdapter(
            &adapterOptions,
            wgpu::CallbackMode::WaitAnyOnly,
            [](wgpu::RequestAdapterStatus status,
               wgpu::Adapter adapter,
               wgpu::StringView message,
               RHI* userdata)
            {
                if (status == wgpu::RequestAdapterStatus::Success)
                {
                    userdata->Adapter = adapter;
                }
                else
                {
                    Logs::Error("Failed to get adapter: %.*s\n", (int)message.length, message.data);
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
        Logs::Warning("Adapter: %.*s", (int)adapterInfo.device.length, adapterInfo.device.data);

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

        wgpu::Future deviceFuture = Adapter.RequestDevice(
            &deviceDesc,
            wgpu::CallbackMode::WaitAnyOnly,
            [](wgpu::RequestDeviceStatus status,
               wgpu::Device receivedDevice,
               wgpu::StringView message,
               RHI* userdata)
            {
                if (status == wgpu::RequestDeviceStatus::Success)
                {
                    userdata->Device = receivedDevice;
                }
                else
                {
                    Logs::Error("Failed to get device: %.*s\n", (int)message.length, message.data);
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

        for (size_t i = 0; i < capabilities.presentModeCount; i++)
        {
            GUserSettings->SupportedPresentMode.insert(capabilities.presentModes[i]);
        }

        wgpu::PresentMode presentMode = GUserSettings->SupportedPresentMode.size() > 0 ? *GUserSettings->SupportedPresentMode.begin() : wgpu::PresentMode::Immediate;

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

        GCamera = MakeShared<WGCamera>(Device, Queue, Window);
        Multisample->Init(Device, SurfaceConfig);
        DepthBuffer->Init(Device, SurfaceConfig);

        Imgui = MakeUnique<WGImgui>();
        Imgui->Init(Window, Device.Get(), (WGPUTextureFormat)SurfaceFormat);

        return true;
    }

    void Deinit()
    {
        if (Imgui) Imgui->Deinit();
        if (DepthBuffer) DepthBuffer->Deinit();
        if (Multisample) Multisample->Deinit();
        Device = nullptr;
        Adapter = nullptr;
        Instance = nullptr;
        Surface = nullptr;
        if (Window) SDL_DestroyWindow(Window);
        Window = nullptr;
        if (SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)
        {
            SDL_Quit();
        }
    }

    void Pool()
    {
        GTick->UpdateCurrentTick();

        uint64_t frameTime = 0;
        if (GUserSettings->VSync == wgpu::PresentMode::Immediate && GUserSettings->GetFramerateLimit() > 0)
        {
            frameTime = 1000000000 / GUserSettings->GetFramerateLimit();
        }

        if (GUserSettings->bMSAAEnabled != PreviousMSAAEnabled)
        {
            PreviousMSAAEnabled = GUserSettings->bMSAAEnabled;
            Multisample->Init(Device, SurfaceConfig);
            DepthBuffer->Init(Device, SurfaceConfig);
            if (Imgui)
                Imgui->Reinit(Window, Device.Get(), (WGPUTextureFormat)SurfaceFormat);
            if (OnMsaaEnabledChange)
                OnMsaaEnabledChange(Multisample);
        }
        if (GUserSettings->Anisotropic != PreviousAnisotropic)
        {
            PreviousAnisotropic = GUserSettings->Anisotropic;
            if (OnAnisatropicChange)
                OnAnisatropicChange();
        }
        GEvent->Run(Imgui->IsInitialized());
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

        if (GUserSettings->VSync != SurfaceConfig.presentMode)
        {
            if (GUserSettings->SupportedPresentMode.find(GUserSettings->VSync) != GUserSettings->SupportedPresentMode.end())
            {
                SurfaceConfig.presentMode = GUserSettings->VSync;
            }
            else
            {
                Logs::Error("Unsupported present mode: %d", (int)GUserSettings->VSync);
                GUserSettings->VSync = SurfaceConfig.presentMode;
            }
            ConfigureSurface();
            return;
        }

        wgpu::TextureView view = output.texture.CreateView();

        wgpu::CommandEncoder encoder = Device.CreateCommandEncoder();

        wgpu::RenderPassColorAttachment colorAttachment = {
            .view = GUserSettings->bMSAAEnabled ? Multisample->TextureView : view,
            .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
            .resolveTarget = GUserSettings->bMSAAEnabled ? view : nullptr,
            .loadOp = wgpu::LoadOp::Clear,
            .storeOp = GUserSettings->bMSAAEnabled ? wgpu::StoreOp::Discard : wgpu::StoreOp::Store,
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

        if (Imgui->IsInitialized())
        {
            ImGui_ImplWGPU_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();

            if (OnImguiRender) OnImguiRender(ImGui::GetIO());

            ImGui::Render();    

            ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass.Get());
        }

        pass.End();

        wgpu::CommandBuffer cmdBuffer = encoder.Finish();
        Queue.Submit(1, &cmdBuffer);
        Surface.Present();
    }
};
