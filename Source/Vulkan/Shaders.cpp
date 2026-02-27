#include "Vulkan/Shaders.h"
#include "Vulkan/VertexBuffer.h"
#include "Engine/Engine.h"
#include "Utils/System.h"
#include "Utils/Logs.h"
#include <slang.h>
#include <slang-com-ptr.h>

VKShaders::~VKShaders()
{
    Deinit();
}

bool VKShaders::Init(const std::filesystem::path& filepath)
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

    CreateGraphicsPipeline(shaderStages);

    return true;
}

void VKShaders::Deinit()
{
    ShaderModules.clear();
    GraphicsPipeline = nullptr;
    PipelineLayout = nullptr;
}

void VKShaders::Bind()
{
    auto& commandBuffer = GPipeline->GetCommandBuffer();

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *GraphicsPipeline);
}

vk::raii::Pipeline& VKShaders::GetGraphicsPipeline()
{
    return GraphicsPipeline;
}

void VKShaders::CreateGraphicsPipeline(const std::vector<vk::PipelineShaderStageCreateInfo>& shaderStages)
{
    auto bindingDescription = VKVertex::GetBindingDescription();
	auto attributeDescriptions = VKVertex::GetAttributeDescriptions();
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
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.depthBiasEnable = vk::False;
    rasterizer.depthBiasSlopeFactor = 1.0f;
    rasterizer.lineWidth = 1.0f;

    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampling.sampleShadingEnable = vk::False;

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

    PipelineLayout = vk::raii::PipelineLayout(GPipeline->GetDevice(), pipelineLayoutInfo);

    auto& swapChainSurf = GPipeline->GetSwapChainSurfaceFormat();

    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo;
    pipelineRenderingCreateInfo.colorAttachmentCount = 1;
    pipelineRenderingCreateInfo.pColorAttachmentFormats = &swapChainSurf.format;

    vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
    graphicsPipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;
    graphicsPipelineCreateInfo.stageCount = shaderStages.size();
    graphicsPipelineCreateInfo.pStages = shaderStages.data();
    graphicsPipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    graphicsPipelineCreateInfo.pViewportState = &viewportState;
    graphicsPipelineCreateInfo.pRasterizationState = &rasterizer;
    graphicsPipelineCreateInfo.pMultisampleState = &multisampling;
    graphicsPipelineCreateInfo.pColorBlendState = &colorBlending;
    graphicsPipelineCreateInfo.pDynamicState = &dynamicState;
    graphicsPipelineCreateInfo.layout = PipelineLayout;
    graphicsPipelineCreateInfo.renderPass = nullptr;

    GraphicsPipeline = vk::raii::Pipeline(GPipeline->GetDevice(), nullptr, graphicsPipelineCreateInfo);
}

vk::raii::ShaderModule VKShaders::CreateShaderModule(const uint32_t* data, size_t size)
{
    vk::ShaderModuleCreateInfo createInfo;
    createInfo.codeSize = size;
    createInfo.pCode = data;

    vk::raii::ShaderModule shaderModule{
        GPipeline->GetDevice(),
        createInfo
    };

    return shaderModule;
}
