#pragma once

//#include "Config.hpp"
#include "Runtime/Vulkan/Pipeline/Instance.hpp"
#include "Runtime/Vulkan/Pipeline/PhysicalDevice.hpp"
#include "Runtime/Vulkan/Pipeline/Window.hpp"
#include "Runtime/Vulkan/Pipeline/LogicalDevices.hpp"
#include "Runtime/Vulkan/Pipeline/Render.hpp"
#include "Engine/Core/Object.hpp"
#include "Runtime/Utils/Logs.hpp"
#include "Runtime/Settings.hpp"
#include "Runtime/Event.hpp"
#include "Runtime/Tick.hpp"
#include "Runtime/Input.hpp"
#include "Camera.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_raii.hpp>
#include <functional>

class Pipeline : public Object
{
public:
    // Globals
    SharedPtr<GameUserSettings> GUserSettings = nullptr;
    SharedPtr<Event> GEvent = nullptr;
    SharedPtr<Tick> GTick = nullptr;
    SharedPtr<Input> GInput = nullptr;
    SharedPtr<VKCamera> GCamera = nullptr;

    // Envents
    std::function<void(uint32_t)> OnSwap;
    std::function<void(uint32_t)> OnRender;

    // Vulkan;
    UniquePtr<VKInstance> Instance = nullptr;
    UniquePtr<VKPhysicalDevice> PhysicalDevice = nullptr;
    UniquePtr<VKWindow> Window = nullptr;
    UniquePtr<VKLogicalDevice> LogicalDevice = nullptr;
    UniquePtr<VKRender> Render = nullptr;

    vk::raii::CommandPool CommandPool = nullptr;

    Pipeline()
    {
        GUserSettings = MakeShared<GameUserSettings>();
        GEvent = MakeShared<Event>();
        GTick = MakeShared<Tick>();
        GInput = MakeShared<Input>();
    }

    ~Pipeline() override
    {
        Deinit();
    }

    bool Init()
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

        if (!SDL_Vulkan_LoadLibrary(NULL))
        {
            Logs::SdlError();
            SDL_Quit();
            return false;
        }

        Instance = MakeUnique<VKInstance>();
        if (!Instance->Init())
        {
            Deinit();
            return false;
        }

        PhysicalDevice = MakeUnique<VKPhysicalDevice>(Instance->Get());
        if (!PhysicalDevice->Init())
        {
            Deinit();
            return false;
        }

        Window = MakeUnique<VKWindow>(Instance->Get(), PhysicalDevice->Get());
        if (!Window->Init())
        {
            Deinit();
            return false;
        }

        LogicalDevice = MakeUnique<VKLogicalDevice>(PhysicalDevice->Get(), Window->Surface);
        if (!LogicalDevice->Init())
        {
            Deinit();
            return false;
        }

        Render = MakeUnique<VKRender>(GUserSettings, GTick, GInput, PhysicalDevice->Get(), LogicalDevice->Get(), LogicalDevice->Queue, Window->Get(), Window->Surface, CommandPool, OnSwap, OnRender);
        Render->Init();

        CreateCommandPool();
        Render->CreateCommandBuffers();
        Render->CreateSyncObjects();

        GCamera = MakeShared<VKCamera>(Window->Get(), GInput);

        return true;
    }

    void Deinit()
    {
        Instance->DebugMessenger.release();
        Instance->Deinit();
        PhysicalDevice->Deinit();
        Window->Deinit();

        SDL_Vulkan_UnloadLibrary();

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
        GCamera->UpdateMatrix(60.0f, 0.1f, 100.0f);
        Render->DrawFrame();
        GTick->UpdateDeltaTime();
    }

private:
    void CreateCommandPool()
    {
        vk::CommandPoolCreateInfo poolInfo;
        poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        poolInfo.queueFamilyIndex = LogicalDevice->QueueIndex;
        CommandPool = vk::raii::CommandPool(LogicalDevice->Get(), poolInfo);
    }
};
