#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/Io/ThreadPool.hpp"
#include "Runtime/Utils/Logs.hpp"
#include <webgpu/wgpu.h>

class WGAdapter : Object
{
private:
    WGPUAdapter Adapter;
    bool Ready = false;

public:
    WGAdapter() = default;

    ~WGAdapter()
    {
        Deinit();
    }

    bool Init(WGPUInstance instance, WGPUSurface surface, bool highPerformanceAdapter = true)
    {
        WGPURequestAdapterOptions adapterOptions = {};
        adapterOptions.compatibleSurface = surface;
        adapterOptions.powerPreference = highPerformanceAdapter
                                             ? WGPUPowerPreference_HighPerformance
                                             : WGPUPowerPreference_LowPower;

        WGPURequestAdapterCallbackInfo adapterCallback = {
            .mode = WGPUCallbackMode_AllowProcessEvents,
            .callback = OnAdapterRequest,
            .userdata1 = this
        };

        WGPUFuture adapterFuture = wgpuInstanceRequestAdapter(
            instance,
            &adapterOptions,
            adapterCallback
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
        /*
        while (!AdapterReady)
        {
            wgpuInstanceProcessEvents(Instance);
        }
        */
        /*
        WGPUFutureWaitInfo adapterWaitInfo = {};
        adapterWaitInfo.future = adapterFuture;
        adapterWaitInfo.completed = false;

        WGPUWaitStatus adapterWaitStatus = wgpuInstanceWaitAny(Instance, 1, &adapterWaitInfo, 0);
        */

        /*
        if (adapterWaitStatus != WGPUWaitStatus_Success)
        {
            Logs::Error("Adapter not synchronized");
            return false;
        }
        */
    }

    void Deinit()
    {
        if (Adapter) wgpuAdapterRelease(Adapter);
        Ready = false;
    }

    WGPUAdapter& Get() { return Adapter; }

private:
    static void OnAdapterRequest(
        WGPURequestAdapterStatus status,
        WGPUAdapter adapter,
        WGPUStringView message,
        void* userdata1,
        void* userdata2)
    {
        WGAdapter* ctx = static_cast<WGAdapter*>(userdata1);

        if (status != WGPURequestAdapterStatus_Success)
        {
            Logs::Error("Failed to create adapter: %.*s\n", (int)message.length, message.data);

            ctx->Ready = false;
            return;
        }

        ctx->Adapter = adapter;
        ctx->Ready = true;
    }
};
