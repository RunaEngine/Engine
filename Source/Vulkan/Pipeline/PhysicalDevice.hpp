#pragma once

#include "Engine/Core/Object.hpp"
#include "Utils/Logs.hpp"
#include <vulkan/vulkan_raii.hpp>
#include <vector>
#include <functional>

const std::vector<const char*> RequiredDeviceExtension = {
    vk::KHRSwapchainExtensionName
};

class VKPhysicalDevice : public Object
{
private:
    vk::raii::Instance& Instance;
    vk::raii::PhysicalDevice PhysicalDevice = nullptr;
public:
    std::vector<vk::raii::PhysicalDevice> PhysicalDevices;

    VKPhysicalDevice() = default;
    VKPhysicalDevice(vk::raii::Instance& instance) : Instance(instance) {}
    virtual ~VKPhysicalDevice()
    {
        Deinit();
    }

    bool Init(const std::vector<const char*>& requiredDeviceExtension = RequiredDeviceExtension)
    {
        std::vector<vk::raii::PhysicalDevice> devices = Instance.enumeratePhysicalDevices();
        if (devices.empty())
        {
            Logs::Error("Failed to find GPUs with Vulkan support");
            return false;
        }
        // Use an ordered map to automatically sort candidates by increasing score
        for (const auto& device : devices)
        {
            // Check if the device supports the Vulkan 1.4 API version
            bool supportsVulkan = device.getProperties().apiVersion >= VK_API_VERSION_1_4;

            // Check if any of the queue families support graphics operations
            auto queueFamilies = device.getQueueFamilyProperties();
            bool supportsGraphics = std::ranges::any_of(queueFamilies, [](auto const& qfp)
                {
                    return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics);
                });

            // Check if all required device extensions are available
            auto availableDeviceExtensions = device.enumerateDeviceExtensionProperties();
            bool supportsAllRequiredExtensions = std::ranges::all_of(requiredDeviceExtension,
                [&availableDeviceExtensions](
                    auto const& requiredDeviceExtension)
                {
                    return std::ranges::any_of(
                        availableDeviceExtensions,
                        [requiredDeviceExtension](
                            auto const& availableDeviceExtension)
                        {
                            return strcmp(
                                availableDeviceExtension.
                                extensionName,
                                requiredDeviceExtension) == 0;
                        });
                });

            auto features = device.template getFeatures2<
                vk::PhysicalDeviceFeatures2,
                vk::PhysicalDeviceVulkan11Features,
                vk::PhysicalDeviceVulkan13Features,
                vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

            bool supportsRequiredFeatures = features.template get<
                vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
                features.template get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
                features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
                features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

            if (supportsVulkan && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures)
            {
                PhysicalDevices.push_back(device);
            }
        }

        if (PhysicalDevices.empty())
        {
            Logs::Error("Failed to find a suitable GPU");
            return false;
        }

        if (OnEnumaratedPhysicalDevices)
            OnEnumaratedPhysicalDevices(PhysicalDevices, PhysicalDevice);

        if (PhysicalDevice == nullptr)
            PhysicalDevice = PhysicalDevices[0];

        Logs::Warning("Device selected: %s", PhysicalDevice.getProperties().deviceName.data());

        return true;
    }

    void Deinit()
    {
        PhysicalDevices.clear();
        PhysicalDevices.shrink_to_fit();
        PhysicalDevice.release();
    }

    vk::raii::PhysicalDevice& Get()
    {
        return PhysicalDevice;
    }

    std::function<void(std::vector<vk::raii::PhysicalDevice>&, vk::raii::PhysicalDevice&)> OnEnumaratedPhysicalDevices;
};
