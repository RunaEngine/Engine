#pragma once

#if defined(SDL_PLATFORM_MACOS) || defined(SDL_PLATFORM_IOS)
#  include <QuartzCore/CAMetalLayer.h>
#  if defined(SDL_PLATFORM_MACOS)
#    include <Cocoa/Cocoa.h>
#  else
#    include <UIKit/UIKit.h>
#  endif
#elif defined(SDL_PLATFORM_WIN32)
#  include <windows.h>
#endif
#include <string>

#include <webgpu/webgpu_cpp.h>
#include <dawn/native/DawnNative.h>
#include <SDL3/SDL_vulkan.h>

#if defined(_WIN32)
#include <dawn/native/D3D12Backend.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <windows.h>
using Microsoft::WRL::ComPtr;
#endif

#if defined(__linux__) || defined(_WIN32) || defined(__ANDROID__)
#include <dawn/native/VulkanBackend.h>
#endif

#if defined(__APPLE__)
#include <dawn/native/MetalBackend.h>
#import <Metal/Metal.h>
#include <sys/sysctl.h>
#endif

#if defined(__linux__)
#include <sys/sysinfo.h>
#endif

#include <iostream>
#include <cstdint>

namespace WGPUtils {
    struct VRAMInfo {
        uint64_t VideoRam = 0;
        uint64_t Budget = 0;
        uint64_t Usage = 0;
        bool bIsIntegrated = false;
        bool bIsEstimated = false;
    };

#if defined(_WIN32)
    VRAMInfo QueryVRAM_D3D12(wgpu::Device device) {
        VRAMInfo info;

        ComPtr<ID3D12Device> d3dDevice = dawn::native::d3d12::GetD3D12Device(device.Get());
        if (!d3dDevice) return info;

        // (iGPU Intel/AMD, or dGPU with resizable BAR)
        D3D12_FEATURE_DATA_ARCHITECTURE arch = {};
        if (SUCCEEDED(d3dDevice->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &arch, sizeof(arch)))) {
            info.bIsIntegrated = arch.UMA;
        }

        ComPtr<IDXGIFactory4> dxgiFactory;
        CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));

        LUID adapterLuid = d3dDevice->GetAdapterLuid();
        ComPtr<IDXGIAdapter1> adapter;
        for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);
            if (memcmp(&desc.AdapterLuid, &adapterLuid, sizeof(LUID)) == 0)
                break;
            adapter.Reset();
        }

        if (adapter) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);
            info.VideoRam = desc.DedicatedVideoMemory;

            ComPtr<IDXGIAdapter3> adapter3;
            if (SUCCEEDED(adapter.As(&adapter3))) {
                DXGI_QUERY_VIDEO_MEMORY_INFO memInfo = {};
                adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memInfo);
                info.Budget = memInfo.Budget;
                info.Usage = memInfo.CurrentUsage;
            }
        }

        if (info.bIsIntegrated) {
            uint64_t heuristic = (SDL_GetSystemRAM() / 2) * 1024 * 1024;
            if (info.Budget < heuristic) {
                info.Budget = heuristic;
                info.bIsEstimated = true;
            }
        }

        return info;
    }
#endif
#if defined(__linux__) || defined(_WIN32) || defined(__ANDROID__)
    VRAMInfo QueryVRAM_Vulkan(wgpu::Device device) {
        VRAMInfo info;

        VkInstance instance = dawn::native::vulkan::GetInstance(device.Get());
        if (!instance) return info;

        wgpu::AdapterInfo adapterInfo = {};
        device.GetAdapterInfo(&adapterInfo);

        // Get function pointers for Vulkan functions
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr();
        auto pfnEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)vkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDevices");
        auto pfnGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties");
        auto pfnGetPhysicalDeviceMemoryProperties2 = (PFN_vkGetPhysicalDeviceMemoryProperties2)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceMemoryProperties2");

        if (!pfnEnumeratePhysicalDevices || !pfnGetPhysicalDeviceProperties || !pfnGetPhysicalDeviceMemoryProperties2) {
            return info;
        }

        // Enumerate physical devices and find the one that matches the adapter info
        uint32_t deviceCount = 0;
        pfnEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) return info;

        std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
        pfnEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

        VkPhysicalDevice vkDevice = nullptr;
        VkPhysicalDeviceProperties vkDeviceProps = {};

        for (auto physDev : physicalDevices) {
            VkPhysicalDeviceProperties props;
            pfnGetPhysicalDeviceProperties(physDev, &props);

            if (props.vendorID == adapterInfo.vendorID && props.deviceID == adapterInfo.deviceID) {
                vkDevice = physDev;
                vkDeviceProps = props;
                break;
            }
        }
        if (!vkDevice) return info;

        // Determine if the device is integrated based on its type
        info.bIsIntegrated = (vkDeviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);

        // Query memory properties using VK_EXT_memory_budget extension
        VkPhysicalDeviceMemoryBudgetPropertiesEXT budgetProps = {};
        budgetProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;

        VkPhysicalDeviceMemoryProperties2 memProps2 = {};
        memProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
        memProps2.pNext = &budgetProps;

        pfnGetPhysicalDeviceMemoryProperties2(vkDevice, &memProps2);

        bool gotBudgetData = false;
        uint64_t videoRamBytes = 0;
        uint64_t budgetBytes = 0;
        uint64_t usageBytes = 0;

        for (uint32_t i = 0; i < memProps2.memoryProperties.memoryHeapCount; i++) {
            // Check if the memory heap is device local (VRAM)
            if (memProps2.memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                videoRamBytes += memProps2.memoryProperties.memoryHeaps[i].size;
                if (budgetProps.heapBudget[i] > 0) {
                    budgetBytes += budgetProps.heapBudget[i];
                    usageBytes += budgetProps.heapUsage[i];
                    gotBudgetData = true;
                }
            }
        }

        // Fallback if VK_EXT_memory_budget is not available or returns zero values
        if (!gotBudgetData) {
            for (uint32_t i = 0; i < memProps2.memoryProperties.memoryHeapCount; i++) {
                if (memProps2.memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                    budgetBytes += memProps2.memoryProperties.memoryHeaps[i].size;
                    videoRamBytes += memProps2.memoryProperties.memoryHeaps[i].size;
                }
            }
            usageBytes = 0;
        }

        info.VideoRam = videoRamBytes;
        info.Budget = budgetBytes;
        info.Usage = usageBytes;

        if (info.bIsIntegrated) {
            uint64_t heuristic = (SDL_GetSystemRAM() / 2) * 1024 * 1024;
            if (info.Budget < heuristic) {
                info.Budget = heuristic;
                info.bIsEstimated = true;
            }
        }

        return info;
    }
#endif
#if defined(__APPLE__)
    VRAMInfo QueryVRAM_Metal(WGPUDevice cDevice) {
        VRAMInfo info;

        id<MTLDevice> mtlDevice = dawn::native::metal::GetMetalDevice(device.Get());
        if (!mtlDevice) return info;

        if (@available(macOS 10.15, iOS 13.0, *)) {
            info.bIsIntegrated = mtlDevice.hasUnifiedMemory;
        }

        info.VideoRam = mtlDevice.recommendedMaxWorkingSetSize;
        info.Budget = mtlDevice.recommendedMaxWorkingSetSize;
        info.Usage = mtlDevice.currentAllocatedSize;

        if (info.bIsIntegrated) {
            uint64_t heuristic = (SDL_GetSystemRAM() / 2) * 1024 * 1024;
            if (info.Budget < heuristic) {
                info.Budget = heuristic;
                info.bIsEstimated = true;
            }
        }

        return info;
    }
#endif

    wgpu::StringView StrToWgpuStringView(const std::string& str)
    {
        wgpu::StringView strView = {};
        strView.data = str.data();
        strView.length = str.size();

        return strView;
    }

    std::string WgpuStringViewToStr(wgpu::StringView& strView)
    {
        std::string str(strView.data, strView.length);
        return str;
    }

    wgpu::TextureFormat RemoveSrgbSuffix(wgpu::TextureFormat format) {
        switch (format) {
        case wgpu::TextureFormat::RGBA8UnormSrgb:
            return wgpu::TextureFormat::RGBA8Unorm;
        case wgpu::TextureFormat::BGRA8UnormSrgb:
            return wgpu::TextureFormat::BGRA8Unorm;
        case wgpu::TextureFormat::BC1RGBAUnormSrgb:
            return wgpu::TextureFormat::BC1RGBAUnorm;
        case wgpu::TextureFormat::BC2RGBAUnormSrgb:
            return wgpu::TextureFormat::BC2RGBAUnorm;
        case wgpu::TextureFormat::BC3RGBAUnormSrgb:
            return wgpu::TextureFormat::BC3RGBAUnorm;
        case wgpu::TextureFormat::BC7RGBAUnormSrgb:
            return wgpu::TextureFormat::BC7RGBAUnorm;
        case wgpu::TextureFormat::ETC2RGB8UnormSrgb:
            return wgpu::TextureFormat::ETC2RGB8Unorm;
        case wgpu::TextureFormat::ETC2RGB8A1UnormSrgb:
            return wgpu::TextureFormat::ETC2RGB8A1Unorm;
        case wgpu::TextureFormat::ETC2RGBA8UnormSrgb:
            return wgpu::TextureFormat::ETC2RGBA8Unorm;
        case wgpu::TextureFormat::ASTC4x4UnormSrgb:
            return wgpu::TextureFormat::ASTC4x4Unorm;
        default:
            return format;
        }
    }

    wgpu::Surface GetWGPUSurfaceFromSDL3(const wgpu::Instance& instance, SDL_Window* window) {
        SDL_PropertiesID props = SDL_GetWindowProperties(window);

#if defined(SDL_PLATFORM_MACOS)
        {
            NSWindow* nsWindow = (__bridge
                NSWindow*)SDL_GetPointerProperty(
                    props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
            if (!nsWindow) {
                return nullptr;
            }

            [nsWindow.contentView setWantsLayer : YES];
            CAMetalLayer* metalLayer = [CAMetalLayer layer];
            [nsWindow.contentView setLayer : metalLayer] ;

            wgpu::SurfaceSourceMetalLayer metalSource = {};
            metalSource.layer = (__bridge
                void*)metalLayer;

            wgpu::SurfaceDescriptor surfaceDesc = {};
            surfaceDesc.nextInChain = &metalSource;

            return instance.CreateSurface(&surfaceDesc);
        }

#elif defined(SDL_PLATFORM_IOS)
        {
            UIWindow* uiWindow = (__bridge
                UIWindow*)SDL_GetPointerProperty(
                    props, SDL_PROP_WINDOW_UIKIT_WINDOW_POINTER, nullptr);
            if (!uiWindow) {
                return nullptr;
            }

            UIView* uiView = uiWindow.rootViewController.view;
            CAMetalLayer* metalLayer = [CAMetalLayer new];
            metalLayer.opaque = true;
            metalLayer.frame = uiView.frame;
            metalLayer.drawableSize = uiView.frame.size;
            [uiView.layer addSublayer : metalLayer] ;

            wgpu::SurfaceSourceMetalLayer metalSource = {};
            metalSource.layer = (__bridge
                void*)metalLayer;

            wgpu::SurfaceDescriptor surfaceDesc = {};
            surfaceDesc.nextInChain = &metalSource;

            return instance.CreateSurface(&surfaceDesc);
        }

#elif defined(SDL_PLATFORM_WIN32)
        {
            HWND hwnd = (HWND)SDL_GetPointerProperty(
                props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
            if (!hwnd) {
                return nullptr;
            }

            wgpu::SurfaceSourceWindowsHWND hwndSource = {};
            hwndSource.hinstance = GetModuleHandle(nullptr);
            hwndSource.hwnd = hwnd;

            wgpu::SurfaceDescriptor surfaceDesc = {};
            surfaceDesc.nextInChain = &hwndSource;

            return instance.CreateSurface(&surfaceDesc);
        }

#elif defined(SDL_PLATFORM_ANDROID)
        {
            void* nativeWindow = SDL_GetPointerProperty(
                props, SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER, nullptr);
            if (!nativeWindow) {
                return nullptr;
            }

            wgpu::SurfaceSourceAndroidNativeWindow androidSource = {};
            androidSource.window = nativeWindow;

            wgpu::SurfaceDescriptor surfaceDesc = {};
            surfaceDesc.nextInChain = &androidSource;

            return instance.CreateSurface(&surfaceDesc);
        }

#elif defined(SDL_PLATFORM_LINUX)
        {
            const char* videoDriver = SDL_GetCurrentVideoDriver();

            if (videoDriver && SDL_strcmp(videoDriver, "wayland") == 0) {
                struct wl_display* waylandDisplay = (struct wl_display*)SDL_GetPointerProperty(
                    props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
                struct wl_surface* waylandSurface = (struct wl_surface*)SDL_GetPointerProperty(
                    props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);

                if (!waylandDisplay || !waylandSurface) {
                    return nullptr;
                }

                wgpu::SurfaceSourceWaylandSurface waylandSource = {};
                waylandSource.display = waylandDisplay;
                waylandSource.surface = waylandSurface;

                wgpu::SurfaceDescriptor surfaceDesc = {};
                surfaceDesc.nextInChain = &waylandSource;

                return instance.CreateSurface(&surfaceDesc);
            }

            return nullptr;
        }
#elif defined(SDL_PLATFORM_ANDROID)
        {
            void* nativeWindow = SDL_GetPointerProperty(
                props, SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER, nullptr);
            if (!nativeWindow) {
                return nullptr;
            }

            wgpu::SurfaceSourceAndroidNativeWindow androidSource = {};
            androidSource.window = nativeWindow;

            wgpu::SurfaceDescriptor surfaceDesc = {};
            surfaceDesc.nextInChain = &androidSource;

            return instance.CreateSurface(&surfaceDesc);
        }
#elif defined(__EMSCRIPTEN__)
        {
            wgpu::SurfaceSourceCanvasHTMLSelector_Emscripten canvasSource = {};
            canvasSource.selector = "canvas";

            wgpu::SurfaceDescriptor surfaceDesc = {};
            surfaceDesc.nextInChain = &canvasSource;

            return instance.CreateSurface(&surfaceDesc);
        }

#else
#  error "Plataforma nao suportada em GetWGPUSurfaceFromSDL3"
#endif
    }
}
