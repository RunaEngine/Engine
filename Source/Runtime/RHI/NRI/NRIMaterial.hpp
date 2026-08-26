#pragma once
#include "Engine/Core/Object.hpp"
#include "Runtime/RHI/NRI/NRIShader.hpp"
#include "Runtime/RHI/NRI/NRITexture.hpp"
#include "Runtime/RHI/NRI/NRIPipeline.hpp"
#include "Runtime/RHI/NRI/NRIMipmap.hpp"
#include "Runtime/RHI/NRI/NRICamera.hpp"
#include "Runtime/RHI/Utils.hpp"
#include <NRI.h>
#include <vector>


class NRIMaterial : public Object
{
private:
    nri::CoreInterface& ICore;
    nri::Device* Device = nullptr;
    nri::Format& ColorFormat;
    nri::Format& DepthFormat;

    // Fixed: Permanent storage for ranges to prevent memory corruption (dangling pointers) during Pipeline creation
    std::vector<nri::DescriptorRangeDesc> TextureRanges;
    std::vector<nri::DescriptorRangeDesc> CameraRanges;

public:
    inline static UniquePtr<NRIMipmap> MipmapPipeline = nullptr;

    SharedPtr<NRIShader> VertexShader = nullptr;
    SharedPtr<NRIShader> FragmentShader = nullptr;
    UniquePtr<NRIPipeline> Pipeline = nullptr;
    std::vector<SharedPtr<NRITexture>> Textures;
    SharedPtr<NRICamera> Camera = nullptr; // Fixed: Added missing Camera member pointer

    nri::DescriptorPool* DescriptorPool = nullptr;
    nri::DescriptorSet* TextureDescriptorSet = nullptr;
    nri::DescriptorSet* CameraDescriptorSet = nullptr;

    NRIMaterial(nri::CoreInterface& core, nri::Device* device,
                nri::Format& colorFormat, nri::Format& depthFormat,
                SharedPtr<NRIShader> vertexShader, SharedPtr<NRIShader> fragmentShader)
        : ICore(core), Device(device),
          ColorFormat(colorFormat), DepthFormat(depthFormat),
          VertexShader(vertexShader), FragmentShader(fragmentShader)
    {
        if (!MipmapPipeline)
        {
            MipmapPipeline = MakeUnique<NRIMipmap>(core, device);
            SharedPtr<NRIShader> shader = MakeShared<NRIShader>(core, device);
            if (!shader->Init(GetBaseDir().string() + "Resources/Shaders/Mipmap.hlsl", SLANG_STAGE_COMPUTE))
            {
                Logs::RuntimeError("Error initializing compute shader for mipmap generation");
            }
            MipmapPipeline->Init(shader);
        }
    }

    ~NRIMaterial() override { Deinit(); }

    bool Init(SharedPtr<NRICamera> camera, const std::vector<SharedPtr<NRITexture>>& textures = {},
              nri::DescriptorSetDesc* descriptorSetDescs = nullptr)
    {
        Deinit();
        Textures = textures;
        Camera = camera; // Fixed: Save the camera pointer into the member variable

        // Fixed: Pass the correct camera pointer to build layouts safely
        std::vector<nri::DescriptorSetDesc> descriptorSets = BuildDescriptorSetDescs(Camera);

        Pipeline = MakeUnique<NRIPipeline>(ICore, Device, ColorFormat, DepthFormat);
        if (!Pipeline->Init(VertexShader, FragmentShader, descriptorSets))
            return false;

        if (Textures.empty())
        {
            if (!CreateDescriptorPool())
                return false;

            return CreateCameraDescriptorSet();
        }

        if (!CreateDescriptorPool())
            return false;

        if (!CreateTextureDescriptorSet())
            return false;

        if (!CreateCameraDescriptorSet())
            return false;

        return true;
    }

    void Deinit()
    {
        if (Pipeline)
        {
            Pipeline->Deinit();
            Pipeline = nullptr;
        }

        if (DescriptorPool)
        {
            ICore.DestroyDescriptorPool(DescriptorPool);
            DescriptorPool = nullptr;
        }

        CameraDescriptorSet = nullptr;
        TextureDescriptorSet = nullptr;
        Textures.clear();
        Camera = nullptr;
    }

    void UpdatePipeline()
    {
        for (auto& texture : Textures)
            texture->UpdateSampler();

        std::vector<nri::DescriptorSetDesc> descriptorSets = BuildDescriptorSetDescs(Camera);
        if (!Pipeline || !Pipeline->Init(VertexShader, FragmentShader, descriptorSets))
            return;

        if (Textures.empty())
        {
            TextureDescriptorSet = nullptr;
        }

        if (!CreateDescriptorPool())
            return;

        if (!Textures.empty() && !CreateTextureDescriptorSet())
            return;

        CreateCameraDescriptorSet();
    }

    void Bind(nri::CommandBuffer& commandBuffer)
    {
        if (!Pipeline || !Pipeline->Pipeline) return;

        // FIXED: You must bind the active Descriptor Pool to the command list BEFORE binding any Descriptor Sets
        if (DescriptorPool)
        {
            ICore.CmdSetDescriptorPool(commandBuffer, *DescriptorPool);
        }

        ICore.CmdSetPipelineLayout(commandBuffer, nri::BindPoint::GRAPHICS, *Pipeline->PipelineLayout);
        ICore.CmdSetPipeline(commandBuffer, *Pipeline->Pipeline);

        // Index 0 matches registerSpace = 0 (Texture/Sampler)
        if (TextureDescriptorSet)
        {
            nri::SetDescriptorSetDesc setDesc = {};
            setDesc.setIndex = 0;
            setDesc.descriptorSet = TextureDescriptorSet;
            ICore.CmdSetDescriptorSet(commandBuffer, setDesc);
        }

        // Index 1 matches registerSpace = 1 (Camera Buffer)
        if (CameraDescriptorSet)
        {
            nri::SetDescriptorSetDesc setDesc = {};
            setDesc.setIndex = 1;
            setDesc.descriptorSet = CameraDescriptorSet;
            ICore.CmdSetDescriptorSet(commandBuffer, setDesc);
        }
    }

private:
    bool CreateDescriptorPool()
    {
        if (DescriptorPool)
        {
            ICore.DestroyDescriptorPool(DescriptorPool);
            DescriptorPool = nullptr;
        }

        TextureDescriptorSet = nullptr;
        CameraDescriptorSet = nullptr;

        nri::DescriptorPoolDesc poolDesc = {};
        poolDesc.descriptorSetMaxNum = 2;
        poolDesc.constantBufferMaxNum = 1;
        poolDesc.textureMaxNum = std::max((uint32_t)Textures.size(), 1u); // Avoid zero allocation risks
        poolDesc.samplerMaxNum = std::max((uint32_t)Textures.size(), 1u);

        return ICore.CreateDescriptorPool(*Device, poolDesc, DescriptorPool) == nri::Result::SUCCESS;
    }

    bool CreateTextureDescriptorSet()
    {
        if (Textures.empty() || !DescriptorPool)
            return true;

        if (!Pipeline || !Pipeline->PipelineLayout)
            return false;

        // Texture Group uses index 0 (registerSpace = 0)
        nri::Result result = ICore.AllocateDescriptorSets(
            *DescriptorPool, *Pipeline->PipelineLayout,
            0, &TextureDescriptorSet, 1, 0);

        if (result != nri::Result::SUCCESS)
            return false;

        std::vector<nri::Descriptor*> textureViews;
        std::vector<nri::Descriptor*> samplers;
        for (auto& tex : Textures)
        {
            textureViews.push_back(tex->TextureView);
            samplers.push_back(tex->Sampler);
        }

        nri::UpdateDescriptorRangeDesc updateTextures = {};
        updateTextures.descriptorSet = TextureDescriptorSet;
        updateTextures.rangeIndex = 0;
        updateTextures.baseDescriptor = 0;
        updateTextures.descriptors = textureViews.data();
        updateTextures.descriptorNum = (uint32_t)textureViews.size();

        nri::UpdateDescriptorRangeDesc updateSamplers = {};
        updateSamplers.descriptorSet = TextureDescriptorSet;
        updateSamplers.rangeIndex = 1;
        updateSamplers.baseDescriptor = 0;
        updateSamplers.descriptors = samplers.data();
        updateSamplers.descriptorNum = (uint32_t)samplers.size();

        nri::UpdateDescriptorRangeDesc updates[] = {updateTextures, updateSamplers};
        ICore.UpdateDescriptorRanges(updates, 2);

        return true;
    }

    bool CreateCameraDescriptorSet()
    {
        if (!DescriptorPool || !Camera || !Camera->CameraBufferDescriptor)
            return false;

        if (!Pipeline || !Pipeline->PipelineLayout)
            return false;

        // Camera Group uses index 1 (registerSpace = 1)
        nri::Result result = ICore.AllocateDescriptorSets(
            *DescriptorPool, *Pipeline->PipelineLayout,
            1, &CameraDescriptorSet, 1, 0);

        if (result != nri::Result::SUCCESS)
            return false;

        nri::Descriptor* cameraView = Camera->CameraBufferDescriptor;
        nri::UpdateDescriptorRangeDesc updateCamera = {};
        updateCamera.descriptorSet = CameraDescriptorSet;
        updateCamera.rangeIndex = 0;
        updateCamera.baseDescriptor = 0;
        updateCamera.descriptors = &cameraView;
        updateCamera.descriptorNum = 1;

        ICore.UpdateDescriptorRanges(&updateCamera, 1);
        return true;
    }

    std::vector<nri::DescriptorSetDesc> BuildDescriptorSetDescs(const SharedPtr<NRICamera> camera)
    {
        TextureRanges.clear();
        CameraRanges.clear();

        std::vector<nri::DescriptorSetDesc> setDescs;

        if (!Textures.empty())
        {
            uint32_t textureCount = (uint32_t)Textures.size();

            // Obtém a API gráfica atual do dispositivo
            const nri::DeviceDesc& deviceDesc = ICore.GetDeviceDesc(*Device);
            bool isD3D12 = (deviceDesc.graphicsAPI == nri::GraphicsAPI::D3D12);

            // Binding 0 -> Textures (t0 no HLSL, binding 0 no Vulkan SPIR-V)
            nri::DescriptorRangeDesc textureRange = {};
            textureRange.baseRegisterIndex = 0;
            textureRange.descriptorNum = textureCount;
            textureRange.descriptorType = nri::DescriptorType::TEXTURE;
            textureRange.shaderStages = nri::StageBits::FRAGMENT_SHADER;

            // Configuração Dinâmica do Sampler
            nri::DescriptorRangeDesc samplerRange = {};

            // samplerRange.baseRegisterIndex = isD3D12 ? textureCount : 0;
            samplerRange.baseRegisterIndex = textureCount;

            samplerRange.descriptorNum = textureCount;
            samplerRange.descriptorType = nri::DescriptorType::SAMPLER;
            samplerRange.shaderStages = nri::StageBits::FRAGMENT_SHADER;

            TextureRanges.push_back(textureRange);
            TextureRanges.push_back(samplerRange);

            nri::DescriptorSetDesc textureSetDesc = {};
            textureSetDesc.registerSpace = 0; // space0
            textureSetDesc.ranges = TextureRanges.data();
            textureSetDesc.rangeNum = (uint32_t)TextureRanges.size();
            setDescs.push_back(textureSetDesc);
        }

        if (camera)
        {
            // Binding 0 do space1 -> Camera (b0 no HLSL, binding 0 no espaço 1 do Vulkan)
            nri::DescriptorRangeDesc cameraRange = {};
            cameraRange.baseRegisterIndex = 0;
            cameraRange.descriptorNum = 1;
            cameraRange.descriptorType = nri::DescriptorType::CONSTANT_BUFFER;
            cameraRange.shaderStages = nri::StageBits::VERTEX_SHADER;
            CameraRanges.push_back(cameraRange);

            nri::DescriptorSetDesc cameraSetDesc = {};
            cameraSetDesc.registerSpace = 1; // space1
            cameraSetDesc.ranges = CameraRanges.data();
            cameraSetDesc.rangeNum = (uint32_t)CameraRanges.size();
            setDescs.push_back(cameraSetDesc);
        }

        return setDescs;
    }
};
