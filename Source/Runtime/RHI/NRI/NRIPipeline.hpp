#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/Settings.hpp"
#include "Runtime/RHI/NRI/NRIVertexBuffer.hpp"
#include "Runtime/RHI/NRI/NRIShader.hpp"
#include <NRI.h>
#include <vector>

class NRIPipeline : public Object
{
private:
    nri::CoreInterface& ICore;
    nri::Device* Device = nullptr;
    nri::Format ColorFormat = nri::Format::UNKNOWN;
    nri::Format DepthFormat = nri::Format::UNKNOWN;

public:
    nri::PipelineLayout* PipelineLayout = nullptr;
    nri::Pipeline* Pipeline = nullptr;

    NRIPipeline(nri::CoreInterface& core, nri::Device* device,
        nri::Format colorFormat, nri::Format depthFormat)
        : ICore(core), Device(device),
        ColorFormat(colorFormat), DepthFormat(depthFormat)
    {
    }

    ~NRIPipeline() override { Deinit(); }

    bool Init(SharedPtr<NRIShader> vertexShader, SharedPtr<NRIShader> fragmentShader,
        const std::vector<nri::DescriptorSetDesc>& descriptorSets = {})
    {
        Deinit();

        // Pipeline layout
        nri::PipelineLayoutDesc layoutDesc = {};
        layoutDesc.descriptorSets = descriptorSets.empty() ? nullptr : descriptorSets.data();
        layoutDesc.descriptorSetNum = (uint32_t)descriptorSets.size();
        layoutDesc.shaderStages = nri::StageBits::VERTEX_SHADER | nri::StageBits::FRAGMENT_SHADER;

        if (ICore.CreatePipelineLayout(*Device, layoutDesc, PipelineLayout) != nri::Result::SUCCESS)
        {
            Logs::Error("NRIPipeline: failed to create pipeline layout");
            return false;
        }

        // Vertex input
        uint32_t attrCount = 0;
        const nri::VertexAttributeDesc* attrs = NRIVertex::GetAttributes(attrCount);

        nri::VertexStreamDesc streamDesc = NRIVertex::GetStreamDesc(0);

        nri::VertexInputDesc vertexInput = {};
        vertexInput.attributes = attrs;
        vertexInput.attributeNum = (uint8_t)attrCount;
        vertexInput.streams = &streamDesc;
        vertexInput.streamNum = 1;

        // Input assembly
        nri::InputAssemblyDesc inputAssembly = {};
        inputAssembly.topology = nri::Topology::TRIANGLE_LIST;

        // Rasterization
        nri::RasterizationDesc rasterization = {};
        rasterization.fillMode = nri::FillMode::SOLID;
        rasterization.cullMode = nri::CullMode::BACK;
        rasterization.frontCounterClockwise = true;

        // Multisample
        nri::MultisampleDesc multisample = {};
        multisample.sampleNum = (uint8_t)GUserSettings->MSAACount;
        multisample.sampleMask = nri::ALL;

        // Blend (opaco por padrão, igual ao WebGPU original)
        nri::BlendDesc colorBlend = {};
        colorBlend.srcFactor = nri::BlendFactor::ONE;
        colorBlend.dstFactor = nri::BlendFactor::ZERO;
        colorBlend.op = nri::BlendOp::ADD;

        nri::BlendDesc alphaBlend = {};
        alphaBlend.srcFactor = nri::BlendFactor::ONE;
        alphaBlend.dstFactor = nri::BlendFactor::ZERO;
        alphaBlend.op = nri::BlendOp::ADD;

        nri::ColorAttachmentDesc colorAttachment = {};
        colorAttachment.format = ColorFormat;
        colorAttachment.colorBlend = colorBlend;
        colorAttachment.alphaBlend = alphaBlend;
        colorAttachment.colorWriteMask = nri::ColorWriteBits::RGBA;
        colorAttachment.blendEnabled = false;

        // Depth stencil
        nri::DepthAttachmentDesc depthAttachment = {};
        depthAttachment.compareOp = nri::CompareOp::LESS;
        depthAttachment.write = true;

        nri::OutputMergerDesc outputMerger = {};
        outputMerger.colors = &colorAttachment;
        outputMerger.colorNum = 1;
        outputMerger.depth = depthAttachment;
        outputMerger.depthStencilFormat = DepthFormat;

        // Shaders
        nri::ShaderDesc shaders[] = { vertexShader->ShaderDesc, fragmentShader->ShaderDesc };

        // Pipeline
        nri::GraphicsPipelineDesc pipelineDesc = {};
        pipelineDesc.pipelineLayout = PipelineLayout;
        pipelineDesc.vertexInput = &vertexInput;
        pipelineDesc.inputAssembly = inputAssembly;
        pipelineDesc.rasterization = rasterization;
        pipelineDesc.multisample = GUserSettings->MSAACount != MSAA_DISABLED ? &multisample : nullptr;
        pipelineDesc.outputMerger = outputMerger;
        pipelineDesc.shaders = shaders;
        pipelineDesc.shaderNum = 2;

        if (ICore.CreateGraphicsPipeline(*Device, pipelineDesc, Pipeline) != nri::Result::SUCCESS)
        {
            Logs::Error("NRIPipeline: failed to create graphics pipeline");
            return false;
        }

        return true;
    }

    void Deinit()
    {
        if (Pipeline) { ICore.DestroyPipeline(Pipeline);       Pipeline = nullptr; }
        if (PipelineLayout) { ICore.DestroyPipelineLayout(PipelineLayout); PipelineLayout = nullptr; }
    }
};