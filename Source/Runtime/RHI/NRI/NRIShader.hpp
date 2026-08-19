#pragma once

#include <filesystem>
#include <iostream>

#include "Engine/Core/Object.hpp"
#include "Runtime/Utils/System.hpp"
#include <NRI.h>
#include <slang.h>
#include <vector>


class NRIShader : public Object
{
private:
    nri::CoreInterface& ICore;
    nri::Device* Device = nullptr;
    std::vector<uint8_t> Bytecode;

public:
    nri::ShaderDesc ShaderDesc = {};

    NRIShader(nri::CoreInterface& core, nri::Device* device)
        : ICore(core), Device(device) {}

    ~NRIShader() override
    {
        Deinit();
    }

    bool Init(const std::filesystem::path& filepath)
    {
        std::string filename = filepath.filename().string();
        std::string entryPointName;
        SlangStage stage = SLANG_STAGE_NONE;

        if (filename.find(".vs.") != std::string::npos)
        {
            entryPointName = "vs_main";
            stage = SLANG_STAGE_VERTEX;
            ShaderDesc.stage = nri::StageBits::VERTEX_SHADER;
        }
        else if (filename.find(".fs.") != std::string::npos)
        {
            entryPointName = "fs_main";
            stage = SLANG_STAGE_FRAGMENT;
            ShaderDesc.stage = nri::StageBits::FRAGMENT_SHADER;
        }
        else if (filename.find(".cs.") != std::string::npos)
        {
            entryPointName = "cs_main";
            stage = SLANG_STAGE_COMPUTE;
            ShaderDesc.stage = nri::StageBits::COMPUTE_SHADER;
        }
        else
        {
            Logs::Error("Invalid extension in %s", filepath.string().c_str());
            return false;
        }

        std::string hlslSource;
        if (!ReadTextFile(filepath, hlslSource))
        {
            Logs::Error("Failed to read shader file in %s", filepath.string().c_str());
            return false;
        }

        const nri::DeviceDesc& deviceDesc = ICore.GetDeviceDesc(*Device);
        SlangCompileTarget target = (deviceDesc.graphicsAPI == nri::GraphicsAPI::D3D12)
                                    ? SLANG_DXIL
                                    : SLANG_SPIRV;

        if (!CompileAndExtract(hlslSource, entryPointName.c_str(), stage, target))
        {
            return false;
        }

        ShaderDesc.bytecode = Bytecode.data();
        ShaderDesc.size = Bytecode.size();

        return true;
    }

    void Deinit()
    {
        Bytecode.clear();
        Bytecode.shrink_to_fit();
        ShaderDesc = {};
    }

private:
    bool CompileAndExtract(const std::string& shaderSource, const char* entryPointName, SlangStage stage, SlangCompileTarget target)
    {
        slang::IGlobalSession* globalSession = nullptr;
        if (SLANG_FAILED(createGlobalSession(&globalSession))) return false;

        slang::SessionDesc sessionDesc = {};
        slang::TargetDesc targetDesc = {};
        targetDesc.format = target;
        targetDesc.profile = target == SLANG_DXIL ? globalSession->findProfile("sm_6_6") : globalSession->findProfile("spirv_1_5");

        sessionDesc.targets = &targetDesc;
        sessionDesc.targetCount = 1;

        slang::ISession* session = nullptr;
        globalSession->createSession(sessionDesc, &session);

        slang::ICompileRequest* request = nullptr;
        session->createCompileRequest(&request);

        int translationUnitIndex = request->addTranslationUnit(SLANG_SOURCE_LANGUAGE_HLSL, nullptr);
        request->addTranslationUnitSourceString(translationUnitIndex, "memory_shader.hlsl", shaderSource.c_str());

        int entryPointIndex = request->addEntryPoint(translationUnitIndex, entryPointName, stage);

        if (SLANG_FAILED(request->compile()))
        {
            Logs::Error("Slang Compilation Error:\n%s", request->getDiagnosticOutput());
            request->release();
            session->release();
            globalSession->release();
            return false;
        }

        slang::IBlob* compiledBlob = nullptr;
        request->getEntryPointCodeBlob(entryPointIndex, 0, &compiledBlob);

        if (compiledBlob)
        {
            const uint8_t* rawData = reinterpret_cast<const uint8_t*>(compiledBlob->getBufferPointer());
            size_t rawSize = compiledBlob->getBufferSize();

            Bytecode.assign(rawData, rawData + rawSize);

            compiledBlob->release();
        }

        request->release();
        session->release();
        globalSession->release();

        return !Bytecode.empty();
    }
};
