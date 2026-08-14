#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/RHI/Utils.hpp"
#include <algorithm>
#include <set>

class RHI;

enum EWindowMode : uint8_t
{
    eFullscreen = 0,
    eWindowed = 1
};

enum EAnisotropic : uint8_t
{
    eDisabled = 1,
    e2X = 1,
    e4X = 4,
    e8X = 8,
    e16X = 16,
};

struct VRamInfo
{
	uint64_t TotalVRam = 0;
	uint64_t AvailableVRam = 0;
};

class GameUserSettings : public Object
{
private:
    uint16_t FramerateLimit = 0;
    std::set<wgpu::PresentMode> SupportedPresentMode;
	wgpu::Adapter Adapter = nullptr;
	wgpu::Device Device = nullptr;

    friend class RHI;
public:
    wgpu::PresentMode VSync = wgpu::PresentMode::Fifo;
    bool bMSAAEnabled = false;
    EAnisotropic Anisotropic = eDisabled;
    EWindowMode WindowMode = eWindowed;

    GameUserSettings() = default;

    void SetFramerateLimit(uint16_t value)
    {
        if (value == 0)
        {
            FramerateLimit = 0;
            return;
        }

        FramerateLimit = (uint16_t)std::clamp((int)value, 5, 300);
    }

    uint16_t GetFramerateLimit() const
    {
        return FramerateLimit;
    }

    std::set<wgpu::PresentMode> GetSupportedPresentMode() const
    {
        return SupportedPresentMode;
    }

    wgpu::BackendType GetAdapterBackend() const
    {
        if (!Adapter)
            return wgpu::BackendType::Null;
        wgpu::AdapterInfo adapterInfo = {};
        if (Adapter.GetInfo(&adapterInfo) == wgpu::Status::Error)
            return wgpu::BackendType::Null;
        return adapterInfo.backendType;
    }

    wgpu::AdapterType GetAdapterType() const
    {
        if (!Adapter)
            return wgpu::AdapterType::Unknown;
        wgpu::AdapterInfo adapterInfo = {};
        if (Adapter.GetInfo(&adapterInfo) == wgpu::Status::Error)
			return wgpu::AdapterType::Unknown;
        return adapterInfo.adapterType;
    }

	std::string GetAdapterName() const
	{
		if (!Adapter)
			return "Unknown";
		wgpu::AdapterInfo adapterInfo = {};
        if (Adapter.GetInfo(&adapterInfo) == wgpu::Status::Error)
            return "Unknown";
		return std::string(adapterInfo.device.data, adapterInfo.device.length);
	}

    std::string GetAdapterVendor() const
    {
        if (!Adapter)
            return "Unknown";
        wgpu::AdapterInfo adapterInfo = {};
        if (Adapter.GetInfo(&adapterInfo) == wgpu::Status::Error)
            return "Unknown";
        return std::string(adapterInfo.vendor.data, adapterInfo.vendor.length);
    }

    std::string GetAdapterArch() const
    {
        if (!Adapter)
            return "Unknown";
        wgpu::AdapterInfo adapterInfo = {};
        if (Adapter.GetInfo(&adapterInfo) == wgpu::Status::Error)
            return "Unknown";
        return std::string(adapterInfo.architecture.data, adapterInfo.architecture.length);
    }

    wgpu::Limits GetAdapterLimits() const
    {
        wgpu::Limits adapterLimits = {};
        if (!Adapter)
            return adapterLimits;
        if (Adapter.GetLimits(&adapterLimits) == wgpu::Status::Error)
            return adapterLimits;
        return adapterLimits;
    }

    WGPUtils::VRAMInfo GetAdapterVRamInfo() const
	{
        WGPUtils::VRAMInfo vramInfo = {};
		if (!Device)
			return vramInfo;
        
#if defined(_WIN32)
        switch (GetAdapterBackend())
        {
        case wgpu::BackendType::D3D12:
			return WGPUtils::QueryVRAM_D3D12(Device.Get());
        case wgpu::BackendType::Vulkan:
            return WGPUtils::QueryVRAM_Vulkan(Device.Get());
        default:
            return vramInfo;
        }
#endif
#if defined(__linux__) || defined(__ANDROID__)
        switch (GetAdapterBackend())
        {
        case wgpu::BackendType::Vulkan:
            return WGPUtils::QueryVRAM_Vulkan(Device.Get());
        default:
            return vramInfo;
        }
#endif
#if defined(__APPLE__)
        switch (GetAdapterBackend())
        {
        case wgpu::BackendType::Metal:
            return WGPUtils::QueryVRAM_Metal(Device.Get());
        default:
            return vramInfo;
        }
#endif
	}
};

inline SharedPtr<GameUserSettings> GUserSettings = MakeShared<GameUserSettings>();