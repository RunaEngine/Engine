#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/Io/ThreadPool.hpp"
#include "Runtime/Utils/Logs.hpp"
#include <webgpu/wgpu.h>

class WGDevice : Object
{
private:
    WGPUDevice Device;
    bool Ready = false;

public:
    WGDevice() = default;

    ~WGDevice()
    {
        Deinit();
    }

    bool Init(WGPUInstance instance, WGPUAdapter adapter)
    {
        WGPUFeatureName requiredFeatures[] = {WGPUFeatureName_Depth32FloatStencil8};
        WGPUDeviceDescriptor deviceDesc = {
            .requiredFeatureCount = sizeof(requiredFeatures),
            .requiredFeatures = requiredFeatures,
        };

        WGPURequestDeviceCallbackInfo callbackInfo = {
            .mode = WGPUCallbackMode_AllowProcessEvents,
            .callback = OnDeviceRequest,
            .userdata1 = this
        };

        WGPUFuture deviceFuture = wgpuAdapterRequestDevice(
            adapter,
            &deviceDesc,
            callbackInfo
        );

        auto future = GThreadPool.submit_task([&]()
        {
            while (!Ready)
            {
                wgpuInstanceProcessEvents(instance);
            }
        });

        future.wait();
        return Ready;
    }

    void Deinit()
    {
        wgpuDeviceRelease(Device);
    }

    WGPUDevice Get() { return Device; }

private:
    static void OnDeviceRequest(
        WGPURequestDeviceStatus status,
        WGPUDevice device,
        WGPUStringView message,
        void* userdata1,
        void* userdata2)
    {
        WGDevice* ctx = static_cast<WGDevice*>(userdata1);

        if (status != WGPURequestDeviceStatus_Success)
        {
            Logs::Error("Failed to create device: %.*s\n", (int)message.length, message.data);
            ctx->Ready = false;
            return;
        }

        ctx->Device = device;
        ctx->Ready = true;
    }
};
