#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/Utils/System.hpp"
#include <dawn/webgpu_cpp.h>

class WGShader : Object
{
private:
    wgpu::Device Device;
    inline static std::string ComputeEntry = "compute_mipmap";
    inline static std::string VertexEntry = "vs_main";
    inline static std::string FragmentEntry = "fs_main";

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
        shaderSource.code = WGPUtils::StrToWgpuStringView(wgsl);

        wgpu::ShaderModuleDescriptor shaderDesc = {};
        shaderDesc.label = WGPUtils::StrToWgpuStringView(filename);
        shaderDesc.nextInChain = (wgpu::ChainedStruct*)&shaderSource;
		shaderDesc.label = WGPUtils::StrToWgpuStringView(filename);

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

    const std::string& GetComputeEntry() const
    {
        return ComputeEntry;
    }

    const std::string& GetVertexEntry() const
    {
        return VertexEntry;
    }

    const std::string& GetFragmentEntry() const
    {
        return FragmentEntry;
    }
};
