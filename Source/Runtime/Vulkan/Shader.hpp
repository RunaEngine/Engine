#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/Utils/System.hpp"
#include "Runtime/Utils/Logs.hpp"
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <filesystem>
#include <slang.h>
#include <slang-com-ptr.h>

class VKShader : public Object
{
public:
    vk::raii::PipelineLayout PipelineLayout = nullptr;
    vk::raii::Pipeline GraphicsPipeline = nullptr;
    std::vector<vk::raii::ShaderModule> ShaderModules;
    
    VKShader() = default;

    ~VKShader() override
    {
        Deinit();
    }

    bool Init(const std::filesystem::path& filepath,
        vk::VertexInputBindingDescription& bindingDescription,
        std::array<vk::VertexInputAttributeDescription, 4> attributeDescriptions,
        vk::raii::DescriptorSetLayout& descriptorSetLayout
        )
    {
        ShaderModules.clear();

        auto filename = filepath.filename();
        std::string slangfile;
        if (!ReadTextFile(filepath, slangfile))
        {
            return false;
        }

        Slang::ComPtr<slang::IGlobalSession> globalSession;
        if (SLANG_FAILED(slang::createGlobalSession(globalSession.writeRef())))
        {
            return false;
        }

        // 2. Configurar o Target (SPIR-V)
        slang::TargetDesc targetDesc = {};
        targetDesc.format = SLANG_SPIRV;
        targetDesc.profile = globalSession->findProfile("spirv_1_5");

        slang::SessionDesc sessionDesc = {};
        sessionDesc.targets = &targetDesc;
        sessionDesc.targetCount = 1;

        Slang::ComPtr<slang::ISession> session;
        if (SLANG_FAILED(globalSession->createSession(sessionDesc, session.writeRef())))
        {
            Logs::Error("VULKAN SHADER\nSlang: Failed to create global session");
            return false;
        }

        // 3. Carregar o arquivo .slang como um Módulo
        Slang::ComPtr<slang::IBlob> diagnosticBlob;
        Slang::ComPtr<slang::IModule> module;
        module = session->loadModuleFromSourceString(
            filename.string().c_str(),
            filepath.parent_path().string().c_str(),
            slangfile.c_str(),
            diagnosticBlob.writeRef()
        );

        if (!module)
        {
            Logs::Error("VULKAN SHADER\n%s", (const char*)diagnosticBlob->getBufferPointer());
            return false;
        }

        // 4. Encontrar Entry Points e Compilar para SPIR-V
        Slang::ComPtr<slang::IEntryPoint> vertEntryPoint;
        module->findEntryPointByName("VertexMain", vertEntryPoint.writeRef());

        Slang::ComPtr<slang::IEntryPoint> fragEntryPoint;
        module->findEntryPointByName("FragmentMain", fragEntryPoint.writeRef());

        std::vector<slang::IComponentType*> componentPtrs;
        if (module) componentPtrs.push_back(module.get());
        if (vertEntryPoint) componentPtrs.push_back(vertEntryPoint.get());
        if (fragEntryPoint) componentPtrs.push_back(fragEntryPoint.get());

        Slang::ComPtr<slang::IComponentType> composedProgram;
        if (SLANG_FAILED(session->createCompositeComponentType(
            componentPtrs.data(),
            (uint32_t)componentPtrs.size(),
            composedProgram.writeRef(),
            diagnosticBlob.writeRef())))
        {
            Logs::Error("VULKAN SHADER\n%s", (const char*)diagnosticBlob->getBufferPointer());
            return false;
        }

        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
        shaderStages.reserve(2);
        Slang::ComPtr<slang::IBlob> vertSpvBlob;
        if (SLANG_SUCCEEDED(composedProgram->getEntryPointCode(0, 0, vertSpvBlob.writeRef())))
        {
            ShaderModules.push_back(CreateShaderModule(
                (const uint32_t*)vertSpvBlob->getBufferPointer(),
                vertSpvBlob->getBufferSize()
            ));
            vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
            vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
            vertShaderStageInfo.module = *ShaderModules.back();
            vertShaderStageInfo.pName = "main";
            shaderStages.push_back(vertShaderStageInfo);
        }
        else
        {
            Logs::Error("VULKAN SHADER\nFailed to get spir-v vertex shader code");
            return false;
        }

        Slang::ComPtr<slang::IBlob> fragSpvBlob;
        if (SLANG_SUCCEEDED(composedProgram->getEntryPointCode(1, 0, fragSpvBlob.writeRef())))
        {
            ShaderModules.push_back(CreateShaderModule(
                (const uint32_t*)fragSpvBlob->getBufferPointer(),
                fragSpvBlob->getBufferSize()
            ));
            vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
            fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
            fragShaderStageInfo.module = *ShaderModules.back();
            fragShaderStageInfo.pName = "main";
            shaderStages.push_back(fragShaderStageInfo);
        }
        else
        {
            Logs::Error("VULKAN SHADER\nFailed to get spir-v fragment shader code");
            return false;
        }

        CreateGraphicsPipeline(shaderStages, bindingDescription, attributeDescriptions, descriptorSetLayout);

        return true;
    }

    void Deinit()
    {
        ShaderModules.clear();
        GraphicsPipeline = nullptr;
        PipelineLayout = nullptr;
    }

    void Bind()
    {
        auto& commandBuffer = GPipeline->Render->CommandBuffers[GPipeline->Render->FrameIndex];

        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *GraphicsPipeline);
    }

    void CreateGraphicsPipeline(const std::vector<vk::PipelineShaderStageCreateInfo>& shaderStages,
        const vk::VertexInputBindingDescription& bindingDescription,
        std::array<vk::VertexInputAttributeDescription, 4> attributeDescriptions,
        vk::raii::DescriptorSetLayout& descriptorSetLayout)
    {
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
        inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;

        vk::PipelineViewportStateCreateInfo viewportState;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        vk::PipelineRasterizationStateCreateInfo rasterizer;
        rasterizer.depthClampEnable = vk::False;
        rasterizer.rasterizerDiscardEnable = vk::False;
        rasterizer.polygonMode = vk::PolygonMode::eFill;
        rasterizer.cullMode = vk::CullModeFlagBits::eBack;
        rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
        rasterizer.depthBiasEnable = vk::False;
        rasterizer.depthBiasConstantFactor = 0.0f;
        rasterizer.depthBiasClamp = 0.0f;
        rasterizer.depthBiasSlopeFactor = 1.0f;
        rasterizer.lineWidth = 1.0f;
/*
        vk::PipelineRasterizationStateCreateInfo rasterizer(
            {}, 
            vk::False, 
            vk::False, 
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eBack, 
            vk::FrontFace::eCounterClockwise,
            vk::False, 
            0.0f, 0.0f, 1.0f, 1.0f
        );
*/
        vk::PipelineMultisampleStateCreateInfo multisampling;
        multisampling.rasterizationSamples = GPipeline->Render->MSAASamples;
        multisampling.sampleShadingEnable = vk::True;
        multisampling.minSampleShading = .2f;

        vk::PipelineColorBlendAttachmentState colorBlendAttachment;
        colorBlendAttachment.blendEnable = vk::False;
        colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

        vk::PipelineColorBlendStateCreateInfo colorBlending;
        colorBlending.logicOpEnable = vk::False;
        colorBlending.logicOp = vk::LogicOp::eCopy;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        std::vector<vk::DynamicState> dynamicStates = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };
        vk::PipelineDynamicStateCreateInfo dynamicState;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
        pipelineLayoutInfo.setLayoutCount = 1, pipelineLayoutInfo.pSetLayouts = &*descriptorSetLayout,
            pipelineLayoutInfo.pushConstantRangeCount = 0;

        PipelineLayout = vk::raii::PipelineLayout(GPipeline->LogicalDevice->Get(), pipelineLayoutInfo);

        auto& swapChainSurf = GPipeline->Render->SwapChainSurfaceFormat;

        vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo;
        pipelineRenderingCreateInfo.colorAttachmentCount = 1;
        pipelineRenderingCreateInfo.pColorAttachmentFormats = &swapChainSurf.format;

        vk::Format depthFormat = VKUtils::FindDepthFormat();
        vk::PipelineDepthStencilStateCreateInfo depthStencil;
        depthStencil.depthTestEnable = vk::True;
        depthStencil.depthWriteEnable = vk::True;
        depthStencil.depthCompareOp = vk::CompareOp::eLess;
        depthStencil.depthBoundsTestEnable = vk::False;
        depthStencil.stencilTestEnable = vk::False;

        vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain;
        //pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>().pNext = &pipelineRenderingCreateInfo;
        //pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>().stageCount = shaderStages.size();
        pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>().pStages = shaderStages.data();
        pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>().pVertexInputState = &vertexInputInfo;
        pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>().pInputAssemblyState = &inputAssembly;
        pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>().pViewportState = &viewportState;
        pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>().pRasterizationState = &rasterizer;
        pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>().pMultisampleState = &multisampling;
        pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>().pDepthStencilState = &depthStencil;
        pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>().pColorBlendState = &colorBlending;
        pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>().pDynamicState = &dynamicState;
        pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>().layout = PipelineLayout;
        pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>().renderPass = nullptr;
        pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>().stageCount = 2;
        pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>().pDepthStencilState = &depthStencil;

        pipelineCreateInfoChain.get<vk::PipelineRenderingCreateInfo>().colorAttachmentCount = 1;
        pipelineCreateInfoChain.get<vk::PipelineRenderingCreateInfo>().pColorAttachmentFormats = &GPipeline->Render->SwapChainSurfaceFormat.format;
        pipelineCreateInfoChain.get<vk::PipelineRenderingCreateInfo>().depthAttachmentFormat = depthFormat;

        GraphicsPipeline = vk::raii::Pipeline(GPipeline->LogicalDevice->Get(), nullptr,  pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
    }

    vk::raii::ShaderModule CreateShaderModule(const uint32_t* data, size_t size)
    {
        vk::ShaderModuleCreateInfo createInfo;
        createInfo.codeSize = size;
        createInfo.pCode = data;

        vk::raii::ShaderModule shaderModule{
            GPipeline->LogicalDevice->Get(),
            createInfo
        };

        return shaderModule;
    }
};
