#pragma once

#include "Runtime/Vulkan/Pipeline/PhysicalDevice.hpp"
#include "Engine/Core/Object.hpp"

class VKLogicalDevice : public Object
{
private:
    vk::raii::PhysicalDevice& PhysicalDevice;
    vk::SurfaceKHR& Surface;
    vk::raii::Device Device = nullptr;
public:
    uint32_t QueueIndex = ~0;
    vk::raii::Queue Queue = nullptr;

    VKLogicalDevice() = default;
    VKLogicalDevice(vk::raii::PhysicalDevice& physicalDevice, vk::SurfaceKHR& surface) : PhysicalDevice(physicalDevice), Surface(surface) {}

    bool Init()
    {
        // find the index of the first queue family that supports graphics
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = PhysicalDevice.getQueueFamilyProperties();

        // get the first index into queueFamilyProperties which supports both graphics and present
        for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
        {
            if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
                PhysicalDevice.getSurfaceSupportKHR(qfpIndex, Surface))
            {
                // found a queue family that supports both graphics and present
                QueueIndex = qfpIndex;
                break;
            }
        }
        if (QueueIndex == ~0)
        {
            Logs::Error("Could not find a queue for graphics and present -> terminating");
            return false;
        }

        // query for Vulkan 1.4 features
        vk::StructureChain<
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceVulkan11Features,
            vk::PhysicalDeviceVulkan13Features,
            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
        > featureChain;

        featureChain.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy = true;
        featureChain.get<vk::PhysicalDeviceFeatures2>().features.sampleRateShading = true;
        featureChain.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters = true;
        featureChain.get<vk::PhysicalDeviceVulkan13Features>().synchronization2 = true;
        featureChain.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering = true;
        featureChain.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState = true;

        // create a Device
        float queuePriority = 0.5f;
        vk::DeviceQueueCreateInfo deviceQueueCreateInfo;
        deviceQueueCreateInfo.queueFamilyIndex = QueueIndex;
        deviceQueueCreateInfo.queueCount = 1;
        deviceQueueCreateInfo.pQueuePriorities = &queuePriority;

        vk::DeviceCreateInfo deviceCreateInfo;
        deviceCreateInfo.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>();
        deviceCreateInfo.pEnabledFeatures = nullptr;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(RequiredDeviceExtension.size());
        deviceCreateInfo.ppEnabledExtensionNames = RequiredDeviceExtension.data();

        Device = vk::raii::Device(PhysicalDevice, deviceCreateInfo);
        Queue = vk::raii::Queue(Device, QueueIndex, 0);

        return true;
    }

    void Deinit()
    {
        Queue.release();
        Device.release();
    }

    vk::raii::Device& Get() { return Device; }

    bool SwapSurface(vk::SurfaceKHR& newSurface)
    {
        if (PhysicalDevice.getSurfaceSupportKHR(QueueIndex, newSurface))
        {
            Surface = newSurface;
            return true;
        }

        Logs::Error("A nova janela requer uma fila de apresentacao diferente da atual!");
        return false;
    }
};