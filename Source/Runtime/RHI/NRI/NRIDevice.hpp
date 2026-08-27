#pragma once

#include "Engine/Core/Object.hpp"
#include <NRI.h>
#include <Extensions/NRIDeviceCreation.h>
#include <string>
#include <iostream>

#include "Extensions/NRIWrapperD3D12.h"

class NRIDevice : Object
{
private:
    nri::CoreInterface& ICore;
    nri::Device* Device = nullptr;
public:
    NRIDevice(nri::CoreInterface& icore) : ICore(icore) {}
    virtual ~NRIDevice() override 
    {
        Deinit();
    }

    nri::Result Init(nri::GraphicsAPI graphicsAPI, bool enableValidation = true)
    {
        nri::DeviceCreationDesc deviceDesc = {};
        deviceDesc.graphicsAPI = graphicsAPI;
        // deviceDesc.enableGraphicsAPIValidation = enableValidation;
        deviceDesc.enableNRIValidation = enableValidation;
        deviceDesc.disableD3D12EnhancedBarriers = true;

        nri::CallbackInterface customCallback = {};
        customCallback.MessageCallback = Logger;
        customCallback.userArg = nullptr;

        deviceDesc.callbackInterface = customCallback;
        return nri::nriCreateDevice(deviceDesc, Device);
    }

    void Deinit()
    {
        if (Device)
        {
            nri::nriDestroyDevice(Device);
            Device = nullptr;
        }
    }

    nri::Device* Get() const
    {
        return Device;
    }

    const nri::AdapterDesc& GetAdapterDesc()
    {
        const nri::DeviceDesc& deviceDesc = ICore.GetDeviceDesc(*Device);
        const nri::AdapterDesc& adapterDesc = deviceDesc.adapterDesc;
        return adapterDesc;
    }

private:
    static void Logger(nri::Message messageType, const char* file, uint32_t line, const char* message, void* userArg)
    {
        // Filter out the Vulkan validation false positive caused by the asynchronous streamer mips
        if (message && std::string(message).find("4289779729") != std::string::npos)
        {
            return; // Mute this specific false positive ID completely
        }

        // Forward any other legitimate graphics engine debug logs to the console
        Logs::Warning("[Graphics Debug] Type: %d\nFile: %s\nLine: %d\nMsg: %s", (int)messageType, (file ? file : "Unknown"), line, (message ? message : ""));
    }
};
