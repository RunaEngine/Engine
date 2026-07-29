#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/Settings.hpp"
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_wgpu.h>
#include <SDL3/SDL.h>
#include <dawn/webgpu_cpp.h>

class WGImgui : Object
{
private:
    bool bIsInitialized = false;
    SDL_Window* Window = nullptr;
    WGPUDevice Device = nullptr;
    WGPUTextureFormat SurfaceFormat = WGPUTextureFormat_Undefined;

public:
    WGImgui() = default;
    ~WGImgui() override
    {
        Deinit();
    }

    void Init(SDL_Window* window, WGPUDevice device, WGPUTextureFormat surfaceFormat)
    {
        if (bIsInitialized) return;
        if (!window || !device) return;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        Window = window;
        Device = device;
        SurfaceFormat = surfaceFormat;

        ImGui_ImplWGPU_InitInfo initInfo = {};
        initInfo.Device = device;
        initInfo.RenderTargetFormat = surfaceFormat;
        initInfo.DepthStencilFormat = WGPUTextureFormat_Depth32FloatStencil8;
        initInfo.PipelineMultisampleState.count = GUserSettings->bMSAAEnabled ? uint8_t(4) : uint8_t(1);
        initInfo.PipelineMultisampleState.mask = (uint32_t)~0;
        initInfo.PipelineMultisampleState.alphaToCoverageEnabled = false;

        ImGui_ImplSDL3_InitForOther(window);
        ImGui_ImplWGPU_Init(&initInfo);
        bIsInitialized = true;
    }

    void Reinit(SDL_Window* window, WGPUDevice device, WGPUTextureFormat surfaceFormat)
    {
        if (bIsInitialized)
        {
            Deinit();
        }

        Init(window, device, surfaceFormat);
    }

    void Deinit()
    {
        if (!bIsInitialized) return;
        ImGui_ImplWGPU_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        Window = nullptr;
        Device = nullptr;
        SurfaceFormat = WGPUTextureFormat_Undefined;
        bIsInitialized = false;
    }

    bool IsInitialized() const
    {
        return bIsInitialized;
    }
};
