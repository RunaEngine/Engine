#pragma once

#include "Config.hpp"
#include "Engine/Core/Object.hpp"
#include "Vulkan/Utils.hpp"
#include "Utils/Logs.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_raii.hpp>

class VKWindow : public Object
{
private:
    vk::raii::Instance& Instance;
    vk::raii::PhysicalDevice& PhysicalDevice;
    SDL_Window* Window = nullptr;
public:
    vk::SurfaceKHR Surface = nullptr;

    bool FramebufferResized = false;

    VKWindow() = default;
    VKWindow(vk::raii::Instance& instance, vk::raii::PhysicalDevice& physicalDevice) : Instance(instance), PhysicalDevice(physicalDevice) {}
    ~VKWindow() override
    {
        Deinit();
    }

    bool Init()
    {
        Window = SDL_CreateWindow(
            ENGINE_NAME,
            1024,
            576,
            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
        );

        if (!Window)
        {
            Logs::SdlError();
            return false;
        }

        VkSurfaceKHR surfaceHandle = nullptr;
        if (!SDL_Vulkan_CreateSurface(Window, *Instance, NULL, &surfaceHandle))
        {
            Logs::SdlError();
            return false;
        }
        Surface = surfaceHandle;

        SDL_AddEventWatch(ResizeEventWatcher, this);

        return true;
    }

    void Deinit()
    {
        if (Surface != VK_NULL_HANDLE)
        {
            SDL_Vulkan_DestroySurface(*Instance, Surface, NULL);
            Surface = VK_NULL_HANDLE;
        }

        if (Window != nullptr)
        {
            SDL_DestroyWindow(Window);
            Window = nullptr;
        }
    }

    SDL_Window* Get()
    {
        return Window;
    }

    static bool SDLCALL ResizeEventWatcher(void* userdata, SDL_Event* event)
    {
        VKWindow* self = (VKWindow*)userdata;
        if (event->type == SDL_EVENT_WINDOW_RESIZED)
        {
            //int newWidth = event->window.data1;
            //int newHeight = event->window.data2;
            self->FramebufferResized = true;
        }
        return true;
    }
private:

};