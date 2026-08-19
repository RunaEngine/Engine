#pragma once

#include "Engine/Core/Object.hpp"
#include <NRI.h>
#include <Extensions/NRIDeviceCreation.h>

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
        deviceDesc.enableGraphicsAPIValidation = enableValidation;
        deviceDesc.enableNRIValidation = enableValidation;

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
};