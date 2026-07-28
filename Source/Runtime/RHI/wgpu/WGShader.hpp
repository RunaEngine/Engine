#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/Utils/System.hpp"
#include <dawn/webgpu_cpp.h>

class WGShader : Object
{
private:
    wgpu::Device Device;
    std::string_view VertexEntry = "vs_main";
    std::string_view FragmentEntry = "fs_main";

public:
    wgpu::ShaderModule Shader = nullptr;

    WGShader(wgpu::Device device) : Device(device)
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
        wgpu::ShaderSourceWGSL shaderSource = {};
        shaderSource.sType = wgpu::SType::ShaderSourceWGSL;
        shaderSource.code.data = wgsl.data();
        shaderSource.code.length = wgsl.size();

        wgpu::ShaderModuleDescriptor shaderDesc = {};
        shaderDesc.nextInChain = (wgpu::ChainedStruct*)&shaderSource;
        shaderDesc.label.data = filename.data();
        shaderDesc.label.length = filename.size();

        Shader = Device.CreateShaderModule(&shaderDesc);

        return Shader != nullptr;
    }

    void Deinit()
    {
        Shader = nullptr;
    }

    wgpu::ShaderModule& Get() { return Shader; }

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
