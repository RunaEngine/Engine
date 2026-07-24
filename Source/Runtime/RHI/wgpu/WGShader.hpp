#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/Utils/Logs.hpp"
#include "Runtime/Utils/System.hpp"
#include <webgpu/wgpu.h>

class WGShader : Object
{
private:
    WGPUDevice Device = nullptr;
    WGPUShaderModule Shader = nullptr;
    std::string_view VertexEntry = "vs_main";
    std::string_view FragmentEntry = "fs_main";

public:
    WGShader(WGPUDevice device) : Device(device)
    {
    }

    ~WGShader() override
    {
        Deinit();
    }

    bool Init(const std::filesystem::path& filepath)
    {
        auto filename = filepath.filename().string();
        std::string wgsl;
        if (!ReadTextFile(filepath, wgsl))
        {
            return false;
        }

        WGPUShaderSourceWGSL shaderSource = {
            .chain = {
                .next = nullptr,
                .sType = WGPUSType_ShaderSourceWGSL
            },
            .code = {
                .data = wgsl.data(),
                .length = wgsl.size()
            }
        };

        WGPUShaderModuleDescriptor shaderDesc = {
            .nextInChain = (WGPUChainedStruct*)&shaderSource,
            .label = {
                .data = filename.data(),
                .length = filename.size()
            }
        };

        Shader = wgpuDeviceCreateShaderModule(
            Device,
            &shaderDesc
        );

        return Shader != nullptr;
    }

    void Deinit()
    {
        if (Shader) wgpuShaderModuleRelease(Shader);
        Shader = nullptr;
    }

    WGPUShaderModule Get() { return Shader; }

    bool IsValid() const
    {
        return Shader != nullptr;
    }

    const char* GetVertexEntry() const
    {
        return VertexEntry.data();
    }

    const char* GetFragmentEntry() const
    {
        return FragmentEntry.data();
    }
};
