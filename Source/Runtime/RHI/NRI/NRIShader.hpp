#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/Utils/System.hpp"
#include <NRI.h>
#ifdef None
#undef None
#endif
#include <slang.h>
#include <vector>
#include <filesystem>
#include <iostream>

using namespace slang;

class NRIShader : public Object
{
private:
    nri::CoreInterface& ICore;
    nri::Device* Device = nullptr;
    std::vector<uint8_t> Bytecode;

    static slang::IGlobalSession* GetGlobalSession()
    {
        static slang::IGlobalSession* globalSession = []() -> slang::IGlobalSession*
        {
            slang::IGlobalSession* session = nullptr;
            if (SLANG_FAILED(slang::createGlobalSession(&session)))
            {
                Logs::Error("NRIShader: failed to create Slang global session");
                return nullptr;
            }
            return session;
        }();
        return globalSession;
    }

public:
    nri::ShaderDesc ShaderDesc = {};

    NRIShader(nri::CoreInterface& core, nri::Device* device)
        : ICore(core), Device(device) {}

    ~NRIShader() override
    {
        Deinit();
    }

    bool Init(const std::filesystem::path& filepath, SlangStage stage)
    {
        std::string filename = filepath.filename().string();
        std::string entryPointName;

        if (stage == SLANG_STAGE_VERTEX)
        {
            entryPointName = "vs_main";
            ShaderDesc.stage = nri::StageBits::VERTEX_SHADER;
        }
        else if (stage == SLANG_STAGE_FRAGMENT)
        {
            entryPointName = "fs_main";
            ShaderDesc.stage = nri::StageBits::FRAGMENT_SHADER;
        }
        else if (stage == SLANG_STAGE_COMPUTE)
        {
            entryPointName = "cs_main";
            ShaderDesc.stage = nri::StageBits::COMPUTE_SHADER;
        }
        else
        {
            Logs::Error("Invalid shader stage: %d", stage);
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
        slang::IGlobalSession* globalSession = GetGlobalSession();
        if (SLANG_FAILED(createGlobalSession(&globalSession))) return false;

        SessionDesc sessionDesc = {};
        TargetDesc targetDesc = {};
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

        IBlob* compiledBlob = nullptr;
        
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
        //globalSession->release();

        return !Bytecode.empty();
    }
};
