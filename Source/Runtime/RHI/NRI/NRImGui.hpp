#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/Utils/Logs.hpp"
#include <Extensions/NRIWrapperD3D12.h>
#include <Extensions/NRIWrapperVK.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <SDL3/SDL.h>

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#include <d3d12.h>
#include <dxgiformat.h>
#include <imgui_impl_dx12.h>
#endif
#include <vulkan/vulkan.h>
#include <NRI.h>

#include "SDL3/SDL_vulkan.h"

class NRImGui : public Object
{
private:
    nri::CoreInterface& ICore;
    nri::Device* Device = nullptr;
    bool bIsInitialized = false;

#ifdef _WIN32
    ID3D12DescriptorHeap* D3D12DescHeap = nullptr;
    uint32_t D3D12DescriptorOffset = 0;
    uint32_t D3D12DescriptorCount = 0;
#endif
    VkDescriptorPool VKDescriptorPool = VK_NULL_HANDLE;
    VkFormat VKColorFormat = VK_FORMAT_UNDEFINED;

public:
    NRImGui(nri::CoreInterface& core, nri::Device* device)
        : ICore(core), Device(device)
    {
    }

    ~NRImGui() override
    {
        Deinit();
    }

    NRImGui(const NRImGui&) = delete;
    NRImGui& operator=(const NRImGui&) = delete;

    bool Init(SDL_Window* window, nri::Format swapChainFormat, uint32_t queuedFrameNum, nri::Queue* queue, uint8_t msaaSampleCount)
    {
        if (bIsInitialized) return true;

        // Initialize ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        (void)io;

        ImGui::StyleColorsDark();

        // Initialize ImGui SDL3 backend
        if (!ImGui_ImplSDL3_InitForOther(window))
        {
            Logs::Error("NRImGui: falha ao inicializar backend SDL3.");
            ImGui::DestroyContext();
            return false;
        }

        const nri::DeviceDesc& deviceDesc = ICore.GetDeviceDesc(*Device);
        if (deviceDesc.graphicsAPI == nri::GraphicsAPI::D3D12)
        {
#ifdef _WIN32
            // Get native D3D12 device and queue
            ID3D12Device* d3d12Device = static_cast<ID3D12Device*>(ICore.GetDeviceNativeObject(Device));
            ID3D12CommandQueue* d3d12Queue = static_cast<ID3D12CommandQueue*>(ICore.GetQueueNativeObject(queue));

            if (!d3d12Device || !d3d12Queue)
            {
                Logs::Error("NRImGui: failed to get native D3D12 device or queue.");
                ImGui_ImplSDL3_Shutdown();
                ImGui::DestroyContext();
                return false;
            }

            D3D12DescriptorCount = 64;

            D3D12_DESCRIPTOR_HEAP_DESC desc = {};
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            desc.NumDescriptors = D3D12DescriptorCount;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            desc.NodeMask = 0;

            HRESULT hr = d3d12Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&D3D12DescHeap));
            if (FAILED(hr))
            {
                Logs::Error("NRImGui: failed to create D3D12 descriptor heap: %d", static_cast<int>(hr));
                ImGui_ImplSDL3_Shutdown();
                ImGui::DestroyContext();
                return false;
            }

            ImGui_ImplDX12_InitInfo initInfo = {};
            initInfo.Device = d3d12Device;
            initInfo.CommandQueue = d3d12Queue;
            initInfo.NumFramesInFlight = queuedFrameNum;
            initInfo.RTVFormat = ToDXGIFormat(swapChainFormat);
            initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
            initInfo.SrvDescriptorHeap = D3D12DescHeap;
            initInfo.UserData = this;

            initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle)
                {
                    auto* self = static_cast<NRImGui*>(info->UserData);

                    if (!self || self->D3D12DescriptorOffset >= self->D3D12DescriptorCount)
                    {
                        *outCpuHandle = {};
                        *outGpuHandle = {};
                        return;
                    }

                    const UINT incrementSize = info->Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

                    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = info->SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

                    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = info->SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

                    cpuHandle.ptr += static_cast<SIZE_T>(self->D3D12DescriptorOffset) * incrementSize;
                    gpuHandle.ptr += static_cast<UINT64>(self->D3D12DescriptorOffset) * incrementSize;

                    self->D3D12DescriptorOffset++;

                    *outCpuHandle = cpuHandle;
                    *outGpuHandle = gpuHandle;
                };

            initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE)
                {
                };

            if (!ImGui_ImplDX12_Init(&initInfo))
            {
                Logs::Error("NRImGui: Failed to initialize ImGui D3D12 backend.");
                D3D12DescHeap->Release();
                D3D12DescHeap = nullptr;
                ImGui_ImplSDL3_Shutdown();
                ImGui::DestroyContext();
                return false;
            }
#else
            Logs::Error("NRImGui: backend D3D12 solicitado fora do Windows.");
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext();
            return false;
#endif
        }
        else if (deviceDesc.graphicsAPI == nri::GraphicsAPI::VK)
        {
            nri::WrapperVKInterface nriVK = {};
            nri::Result result = nri::nriGetInterface(*Device, NRI_INTERFACE(nri::WrapperVKInterface), &nriVK);

            if (result != nri::Result::SUCCESS)
            {
                Logs::Error("NRImGui: falha ao obter NRI WrapperVKInterface.");
                ImGui_ImplSDL3_Shutdown();
                ImGui::DestroyContext();
                return false;
            }
            VkDevice vkDevice = static_cast<VkDevice>(ICore.GetDeviceNativeObject(Device));
            VkQueue vkQueue = static_cast<VkQueue>(ICore.GetQueueNativeObject(queue));
            VkInstance vkInstance = static_cast<VkInstance>(nriVK.GetInstanceVK(*Device));

            if (!vkDevice || !vkQueue)
            {
                Logs::Error("NRImGui: Failed to get native Vulkan device or queue.");
                ImGui_ImplSDL3_Shutdown();
                ImGui::DestroyContext();
                return false;
            }

            if (!CreateVulkanDescriptorPool(vkDevice, vkInstance))
            {
                ImGui_ImplSDL3_Shutdown();
                ImGui::DestroyContext();
                return false;
            }

            PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr_Ptr = (PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr();

            auto loaderFunc = [](const char* functionName, void* userData) -> PFN_vkVoidFunction
            {
                PFN_vkGetInstanceProcAddr getProcAddr = (PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr();
                return getProcAddr(static_cast<VkInstance>(userData), functionName);
            };

            if (!ImGui_ImplVulkan_LoadFunctions(VK_API_VERSION_1_3, loaderFunc, vkInstance))
            {
                Logs::Error("NRImGui: Failed to load Vulkan functions for ImGui.");
                ImGui_ImplSDL3_Shutdown();
                ImGui::DestroyContext();
                return false;
            }

            VKColorFormat = ToVkFormat(swapChainFormat);

            VkPipelineRenderingCreateInfoKHR pipelineRenderingInfo = {};
            pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
            pipelineRenderingInfo.colorAttachmentCount = 1;
            pipelineRenderingInfo.pColorAttachmentFormats = &VKColorFormat;
            pipelineRenderingInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;

            ImGui_ImplVulkan_InitInfo initInfo = {};
            initInfo.ApiVersion = VK_API_VERSION_1_3;
            initInfo.Instance = vkInstance;
            initInfo.PhysicalDevice = static_cast<VkPhysicalDevice>(nriVK.GetPhysicalDeviceVK(*Device));
            initInfo.Device = vkDevice;
            initInfo.QueueFamily = nriVK.GetQueueFamilyIndexVK(*queue);
            initInfo.Queue = vkQueue;
            initInfo.PipelineCache = VK_NULL_HANDLE;
            initInfo.DescriptorPool = VKDescriptorPool;
            initInfo.MinImageCount = queuedFrameNum;
            initInfo.ImageCount = queuedFrameNum;
            initInfo.UseDynamicRendering = true;
            initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = pipelineRenderingInfo;
            initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

            if (!ImGui_ImplVulkan_Init(&initInfo))
            {
                Logs::Error("NRImGui: Failed to initialize Vulkan backend.");

                PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr_Ptr = (PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr();
                PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool_Ptr = (PFN_vkDestroyDescriptorPool)vkGetInstanceProcAddr_Ptr(vkInstance, "vkDestroyDescriptorPool");
                vkDestroyDescriptorPool_Ptr(vkDevice, VKDescriptorPool, nullptr);
                VKDescriptorPool = VK_NULL_HANDLE;

                ImGui_ImplSDL3_Shutdown();
                ImGui::DestroyContext();
                return false;
            }
        }
        else
        {
            Logs::Error("NRImGui: Graphics API not supported. Only D3D12 and Vulkan are supported.");
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext();
            return false;
        }

        bIsInitialized = true;
        return true;
    }

    void Deinit()
    {
        if (!bIsInitialized) return;

        const nri::DeviceDesc& deviceDesc = ICore.GetDeviceDesc(*Device);

        if (deviceDesc.graphicsAPI == nri::GraphicsAPI::D3D12)
        {
#ifdef _WIN32
            ImGui_ImplDX12_Shutdown();

            if (D3D12DescHeap)
            {
                D3D12DescHeap->Release();
                D3D12DescHeap = nullptr;
            }

            D3D12DescriptorOffset = 0;
            D3D12DescriptorCount = 0;
#endif
        }
        else if (deviceDesc.graphicsAPI == nri::GraphicsAPI::VK)
        {
            nri::WrapperVKInterface nriVK = {};
            nri::Result result = nri::nriGetInterface(*Device, NRI_INTERFACE(nri::WrapperVKInterface), &nriVK);

            if (result == nri::Result::SUCCESS)
            {
                VkInstance nativeInstance = static_cast<VkInstance>(nriVK.GetInstanceVK(*Device));
                VkDevice vkDevice = static_cast<VkDevice>(ICore.GetDeviceNativeObject(Device));

                ImGui_ImplVulkan_Shutdown();

                if (VKDescriptorPool != VK_NULL_HANDLE)
                {
                    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr_Ptr = (PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr();
                    PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool_Ptr = (PFN_vkDestroyDescriptorPool)vkGetInstanceProcAddr_Ptr(nativeInstance, "vkDestroyDescriptorPool");

                    if (vkDestroyDescriptorPool_Ptr)
                    {
                        vkDestroyDescriptorPool_Ptr(vkDevice, VKDescriptorPool, nullptr);
                    }
                }
            }
            VKDescriptorPool = VK_NULL_HANDLE;
            VKColorFormat = VK_FORMAT_UNDEFINED;
        }

        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();

        bIsInitialized = false;
    }

    void BeginFrame()
    {
        if (!bIsInitialized) return;

        const nri::DeviceDesc& deviceDesc = ICore.GetDeviceDesc(*Device);

        if (deviceDesc.graphicsAPI == nri::GraphicsAPI::D3D12)
        {
#ifdef _WIN32
            ImGui_ImplDX12_NewFrame();
#endif
        }
        else if (deviceDesc.graphicsAPI == nri::GraphicsAPI::VK)
        {
            ImGui_ImplVulkan_NewFrame();
        }

        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    void EndAndRender(nri::CommandBuffer* nriCommandBuffer, nri::Descriptor* colorAttachment, uint32_t width, uint32_t height)
    {
        if (!bIsInitialized) return;

        (void)colorAttachment;
        (void)width;
        (void)height;

        ImGui::Render();

        const nri::DeviceDesc& deviceDesc = ICore.GetDeviceDesc(*Device);

        if (deviceDesc.graphicsAPI == nri::GraphicsAPI::D3D12)
        {
#ifdef _WIN32
            ID3D12GraphicsCommandList* nativeCmdList =
                static_cast<ID3D12GraphicsCommandList*>(ICore.GetCommandBufferNativeObject(nriCommandBuffer));

            if (nativeCmdList)
            {
                ID3D12DescriptorHeap* heaps[] = { D3D12DescHeap };
                nativeCmdList->SetDescriptorHeaps(1, heaps);
                ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), nativeCmdList);
            }
#endif
        }
        else if (deviceDesc.graphicsAPI == nri::GraphicsAPI::VK)
        {
            VkCommandBuffer nativeCmdList = static_cast<VkCommandBuffer>(ICore.GetCommandBufferNativeObject(nriCommandBuffer));

            if (nativeCmdList)
            {
                ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), nativeCmdList);
            }
        }
    }

    bool IsInitialized() const
    {
        return bIsInitialized;
    }
private:
#ifdef _WIN32
    static DXGI_FORMAT ToDXGIFormat(nri::Format format)
    {
        switch (format)
        {
        case nri::Format::RGBA8_UNORM:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case nri::Format::RGBA8_SRGB:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case nri::Format::BGRA8_UNORM:
            return DXGI_FORMAT_B8G8R8A8_UNORM;
        case nri::Format::BGRA8_SRGB:
            return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        case nri::Format::RGBA16_SFLOAT:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case nri::Format::RGBA32_SFLOAT:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        default:
            Logs::Warning("NRImGui: formato NRI nao mapeado para DXGI_FORMAT. Usando RGBA8_UNORM.");
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        }
    }
#endif

    static VkFormat ToVkFormat(nri::Format format)
    {
        switch (format)
        {
        case nri::Format::RGBA8_UNORM:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case nri::Format::RGBA8_SRGB:
            return VK_FORMAT_R8G8B8A8_SRGB;
        case nri::Format::BGRA8_UNORM:
            return VK_FORMAT_B8G8R8A8_UNORM;
        case nri::Format::BGRA8_SRGB:
            return VK_FORMAT_B8G8R8A8_SRGB;
        case nri::Format::RGBA16_SFLOAT:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        case nri::Format::RGBA32_SFLOAT:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        default:
            Logs::Warning("NRImGui: formato NRI nao mapeado para VkFormat. Usando RGBA8_UNORM.");
            return VK_FORMAT_R8G8B8A8_UNORM;
        }
    }

    static VkSampleCountFlagBits ToVkSampleCount(uint8_t sampleCount)
    {
        switch (sampleCount)
        {
        case 2:
            return VK_SAMPLE_COUNT_2_BIT;
        case 4:
            return VK_SAMPLE_COUNT_4_BIT;
        case 8:
            return VK_SAMPLE_COUNT_8_BIT;
        default:
            return VK_SAMPLE_COUNT_1_BIT;
        }
    }

    bool CreateVulkanDescriptorPool(VkDevice vkDevice, VkInstance nativeInstance)
    {
        VkDescriptorPoolSize poolSizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1024 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1024 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024 },
        };

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 1024;
        poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
        poolInfo.pPoolSizes = poolSizes;

        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr_Ptr = (PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr();
        PFN_vkCreateDescriptorPool vkCreateDescriptorPool_Ptr = (PFN_vkCreateDescriptorPool)vkGetInstanceProcAddr_Ptr(nativeInstance, "vkCreateDescriptorPool");

        if (!vkCreateDescriptorPool_Ptr)
        {
            Logs::Error("Failed to resolve vkCreateDescriptorPool for imgui.");
            VKDescriptorPool = VK_NULL_HANDLE;
            return false;
        }

        VkResult vr = vkCreateDescriptorPool_Ptr(vkDevice, &poolInfo, nullptr, &VKDescriptorPool);
        if (vr != VK_SUCCESS)
        {
            Logs::Error("Failed to create descriptor pool for imgui (VkResult: %d)", (int)vr);
            VKDescriptorPool = VK_NULL_HANDLE;
            return false;
        }

        return true;
    }
};
