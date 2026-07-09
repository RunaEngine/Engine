#pragma once

#include "Engine/Core/Object.hpp"
#include "Config.hpp"
#include "Runtime/Utils/Logs.hpp"
#include <vulkan/vulkan_raii.hpp>
#include <SDL3/SDL_vulkan.h>

#ifdef NDEBUG
constexpr bool EnableValidationLayers = false;
#else
constexpr bool EnableValidationLayers = true;
#endif

const std::vector<char const*> ValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

class VKInstance : public Object
{
private:
    vk::raii::Instance Instance = nullptr;
public:
    vk::ApplicationInfo AppInfo;
    UniquePtr<vk::raii::Context> Context;
    vk::raii::DebugUtilsMessengerEXT DebugMessenger = nullptr;

    VKInstance() = default;
    ~VKInstance() override
    {
        Deinit();
    }

    bool Init()
    {
        //PFN_vkGetInstanceProcAddr instanceProcAdd = (PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr();
        auto ctx = new vk::raii::Context();
        Context.reset(ctx);

        AppInfo.pApplicationName = ENGINE_NAME;
        AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        AppInfo.pEngineName = "No Engine";
        AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        AppInfo.apiVersion = vk::ApiVersion14;

        // Get the required layers
        std::vector<char const*> requiredLayers;
        bool isLayerSupported = false;
        if (EnableValidationLayers)
        {
            requiredLayers.assign(ValidationLayers.begin(), ValidationLayers.end());
            // Check if the required layers are supported by the Vulkan implementation.
            auto layerProperties = Context->enumerateInstanceLayerProperties();
            auto unsupportedLayerIt = std::ranges::find_if(requiredLayers,
                [&layerProperties](auto const& requiredLayer)
                {
                    return std::ranges::none_of(layerProperties,
                        [requiredLayer](auto const& layerProperty)
                        {
                            return strcmp(
                                layerProperty.layerName,
                                requiredLayer) ==
                                0;
                        });
                });
            isLayerSupported = unsupportedLayerIt == requiredLayers.end();
            if (!isLayerSupported)
            {
                Logs::Warning("Required debug layer not supported: %s", *unsupportedLayerIt);
            }
        }

        // Get the required instance extensions from SDL.
        uint32_t sdlExtensionCount = 0;
        auto sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);

        std::vector<const char*> instanceExtensions;
        for (uint32_t i = 0; i < sdlExtensionCount; ++i)
        {
            instanceExtensions.push_back(sdlExtensions[i]);
        }

        if (EnableValidationLayers)
        {
            instanceExtensions.push_back(vk::EXTDebugUtilsExtensionName);
        }

        // Check if the required extensions are supported by the Vulkan implementation.
        auto extensionProperties = Context->enumerateInstanceExtensionProperties();
        for (auto const& extName : instanceExtensions)
        {
            if (std::ranges::none_of(extensionProperties,
                [extName](auto const& extensionProperty)
                {
                    return strcmp(extensionProperty.extensionName, extName) == 0;
                }))
            {
                Logs::Log("Required extension not supported: %s", extName);
                SDL_Quit();
                return false;
            }
        }

        vk::InstanceCreateInfo createInfo;
        createInfo.pApplicationInfo = &AppInfo;
        if (isLayerSupported)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
                createInfo.ppEnabledLayerNames = requiredLayers.data();
        }
        createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
        createInfo.ppEnabledExtensionNames = instanceExtensions.data();

        Instance = vk::raii::Instance(*Context, createInfo);

        if (isLayerSupported)
            SetupDebugMessenger();

        return true;
    }

    void Deinit()
    {
        DebugMessenger.release();
        Instance.release();
    }

    vk::raii::Instance& Get()
    {
        return Instance;
    }
private:
    void SetupDebugMessenger()
    {
        if (!EnableValidationLayers)
            return;

        vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);

        vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT;
        debugUtilsMessengerCreateInfoEXT.messageSeverity = severityFlags;
        debugUtilsMessengerCreateInfoEXT.messageType = messageTypeFlags;
        debugUtilsMessengerCreateInfoEXT.pfnUserCallback = &DebugCallback;

        DebugMessenger = Instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
    }

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void*)
    {
        if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError || severity ==
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
        {
            std::string debugType;

            if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral)
            {
                debugType += "General ";
            }
            if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
            {
                debugType += "Validation ";
            }
            if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
            {
                debugType += "Performance ";
            }

            if (debugType.empty())
            {
                debugType = "Unknown ";
            }

            Logs::Error("Validation layer: %s\nMessage: %s", debugType.c_str(), pCallbackData->pMessage);
        }

        return vk::False;
    }
};
