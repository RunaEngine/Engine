#pragma once

#include "Engine/Core/Object.hpp"
#include "Vulkan/RenderPass/Color.hpp"
#include "Vulkan/RenderPass/Depth.hpp"
#include "Settings.hpp"
#include "Tick.hpp"
#include "Input.hpp"
#include "SDL3/SDL_video.h"
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vector>
#include <string>

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

class VKRender : public Object
{
private:
    vk::raii::PhysicalDevice& PhysicalDevice;
    vk::raii::Device& Device;
    vk::raii::Queue& Queue;
    SDL_Window* Window = nullptr;
    vk::SurfaceKHR& Surface;
    vk::raii::CommandPool& CommandPool;
public:
    // Globals
    SharedPtr<GameUserSettings>& GUserSettings;
    SharedPtr<Tick>& GTick;
    SharedPtr<Input>& GInput;

    // Events
    std::function<void(uint32_t)>& OnSwap;
    std::function<void(uint32_t)>& OnRender;

    vk::raii::SwapchainKHR SwapChain = nullptr;
    vk::SwapchainCreateInfoKHR SwapChainCreateInfo;
    std::vector<vk::Image> SwapChainImages;
    vk::SurfaceFormatKHR SwapChainSurfaceFormat;
    vk::Extent2D SwapChainExtent;
    std::vector<vk::raii::ImageView> SwapChainImageViews;
    std::vector<vk::raii::Framebuffer> SwapChainFramebuffers;

    std::vector<vk::raii::CommandBuffer> CommandBuffers;

    std::vector<vk::raii::Semaphore> PresentCompleteSemaphores;
    std::vector<vk::raii::Semaphore> RenderFinishedSemaphores;
    std::vector<vk::raii::Fence> InFlightFences;
    uint32_t FrameIndex = 0;
    bool FramebufferResized = false;

    // Render passes
    vk::SampleCountFlagBits MaxMSAASamples = vk::SampleCountFlagBits::e1;
    vk::SampleCountFlagBits MSAASamples = vk::SampleCountFlagBits::e1;
    UniquePtr<VKColor> ColorBuffer = nullptr;
    UniquePtr<VKDepth> DepthBuffer = nullptr;
    vk::raii::RenderPass RenderPass = nullptr;

    VKRender() = default;
    VKRender(SharedPtr<GameUserSettings>& userSettings, SharedPtr<Tick>& tick, SharedPtr<Input>& input, vk::raii::PhysicalDevice& physicalDevice, vk::raii::Device& device, vk::raii::Queue& queue, SDL_Window* window, vk::SurfaceKHR& surface, vk::raii::CommandPool& commandPool, std::function<void(uint32_t)>& onSwap, std::function<void(uint32_t)>& onRender) : GUserSettings(userSettings), GTick(tick), GInput(input), PhysicalDevice(physicalDevice), Device(device), Queue(queue), Window(window), Surface(surface), CommandPool(commandPool), OnSwap(onSwap), OnRender(onRender) {}
    ~VKRender() override
    {
        ColorBuffer->Deinit();
        DepthBuffer->Deinit();
        CleanupSwapChain();
    }

    void Init()
    {
        MaxMSAASamples = VKUtils::GetMaxUsableSampleCount();
        MSAASamples = (vk::SampleCountFlagBits)GUserSettings->MSAASamples;

        CreateSwapChain();
        CreateImageViews();

        // Render passes
        ColorBuffer = MakeUnique<VKColor>(PhysicalDevice, SwapChainSurfaceFormat, SwapChainExtent);
        ColorBuffer->Init(MSAASamples);
        DepthBuffer = MakeUnique<VKDepth>();
        DepthBuffer->Init(SwapChainExtent, MSAASamples);

        CreateRenderPass();
        CreateFramebuffers();
    }

    void CleanupSwapChain()
    {
        DepthBuffer->Deinit();
        SwapChainImageViews.clear();
        SwapChain = nullptr;
    }

    void CreateCommandBuffers()
    {
        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.commandPool = *CommandPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

        CommandBuffers = vk::raii::CommandBuffers(Device, allocInfo);
    }

    void CreateSyncObjects()
    {
        assert(PresentCompleteSemaphores.empty() && RenderFinishedSemaphores.empty() && InFlightFences.empty());

        for (size_t i = 0; i < SwapChainImages.size(); i++)
        {
            RenderFinishedSemaphores.emplace_back(Device, vk::SemaphoreCreateInfo());
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            PresentCompleteSemaphores.emplace_back(Device, vk::SemaphoreCreateInfo());
            vk::FenceCreateInfo createFanceInfo;
            createFanceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
            InFlightFences.emplace_back(Device, createFanceInfo);
        }
    }

    void DrawFrame()
    {
        uint64_t frameTime = 0;
        if (GUserSettings->Vsync != Immediate && GUserSettings->GetFramerateLimit() > 0)
        {
            frameTime = 1000000000 / GUserSettings->GetFramerateLimit();
        }

        auto fenceResult = Device.waitForFences(*InFlightFences[FrameIndex], vk::True, UINT64_MAX);
        if (fenceResult != vk::Result::eSuccess)
        {
            Logs::RuntimeError("%d\nFailed to wait for fence", fenceResult);
            return;
        }

        auto [result, imageIndex] = SwapChain.acquireNextImage(
            UINT64_MAX, *PresentCompleteSemaphores[FrameIndex], nullptr);
        // Due to VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS being defined, eErrorOutOfDateKHR can be checked as a result
        // here and does not need to be caught by an exception.
        if (result == vk::Result::eErrorOutOfDateKHR)
        {
            RecreateSwapChain();
            return;
        }
        // On other success codes than eSuccess and eSuboptimalKHR we just throw an exception.
        // On any error code, aquireNextImage already threw an exception.
        else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
        {
            assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
            Logs::RuntimeError("%d\nFailed to acquire swap chain image", result);
            return;
        }

        if (OnSwap) OnSwap(FrameIndex);

        // Only reset the fence if we are submitting work
        Device.resetFences(*InFlightFences[FrameIndex]);

        CommandBuffers[FrameIndex].reset();
        RecordCommandBuffer(imageIndex);

        vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        vk::SubmitInfo submitInfo;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &*PresentCompleteSemaphores[FrameIndex];
        submitInfo.pWaitDstStageMask = &waitDestinationStageMask;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &*CommandBuffers[FrameIndex];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &*RenderFinishedSemaphores[imageIndex];

        Queue.submit(submitInfo, *InFlightFences[FrameIndex]);

        vk::PresentInfoKHR presentInfoKHR;
        presentInfoKHR.waitSemaphoreCount = 1;
        presentInfoKHR.pWaitSemaphores = &*RenderFinishedSemaphores[imageIndex];
        presentInfoKHR.swapchainCount = 1;
        presentInfoKHR.pSwapchains = &*SwapChain;
        presentInfoKHR.pImageIndices = &imageIndex;

        result = Queue.presentKHR(presentInfoKHR);
        // Due to VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS being defined, eErrorOutOfDateKHR can be checked as a result
        // here and does not need to be caught by an exception.
        if ((result == vk::Result::eSuboptimalKHR) || (result == vk::Result::eErrorOutOfDateKHR) || FramebufferResized)
        {
            FramebufferResized = false;
            RecreateSwapChain();
        }
        else
        {
            // There are no other success codes than eSuccess; on any error code, presentKHR already threw an exception.
            assert(result == vk::Result::eSuccess);
        }

        FrameIndex = (FrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;

        if (frameTime > 0 && frameTime > GTick->ElapsedNS())
        {
            SDL_DelayPrecise(frameTime - GTick->ElapsedNS());
        }
    }

private:
    void CreateSwapChain()
    {
        auto surfaceCapabilities = PhysicalDevice.getSurfaceCapabilitiesKHR(Surface);
        SwapChainExtent = ChooseSwapExtent(surfaceCapabilities);
        SwapChainSurfaceFormat = ChooseSwapSurfaceFormat(PhysicalDevice.getSurfaceFormatsKHR(Surface));

        SwapChainCreateInfo.surface = Surface;
        SwapChainCreateInfo.minImageCount = ChooseSwapMinImageCount(surfaceCapabilities);
        SwapChainCreateInfo.imageFormat = SwapChainSurfaceFormat.format;
        SwapChainCreateInfo.imageColorSpace = SwapChainSurfaceFormat.colorSpace;
        SwapChainCreateInfo.imageExtent = SwapChainExtent;
        SwapChainCreateInfo.imageArrayLayers = 1;
        SwapChainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
        SwapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
        SwapChainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
        SwapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        SwapChainCreateInfo.presentMode = ChooseSwapPresentMode(PhysicalDevice.getSurfacePresentModesKHR(Surface));
        SwapChainCreateInfo.clipped = true;

        if (SwapChainCreateInfo.presentMode != vk::PresentModeKHR::eMailbox && GUserSettings->Vsync == TripleBuffering)
            GUserSettings->Vsync = Adaptative;

        SwapChain = vk::raii::SwapchainKHR(Device, SwapChainCreateInfo);
        SwapChainImages = SwapChain.getImages();
    }

    void CreateRenderPass() {
        bool useMSAA = (MSAASamples != vk::SampleCountFlagBits::e1);

        // 1. Anexo de Cor Principal
        vk::AttachmentDescription colorAttachment;
        colorAttachment.format = SwapChainSurfaceFormat.format;
        colorAttachment.samples = MSAASamples;
        colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
        colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
        colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
        // Se NÃO usa MSAA, o destino final já é o Present do Swapchain
        colorAttachment.finalLayout = useMSAA ? vk::ImageLayout::eColorAttachmentOptimal : vk::ImageLayout::ePresentSrcKHR;

        vk::AttachmentReference colorAttachmentRef;
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

        // 2. Anexo de Profundidade
        vk::AttachmentDescription depthAttachment;
        depthAttachment.format = VKUtils::FindDepthFormat();
        depthAttachment.samples = MSAASamples;
        depthAttachment.loadOp = vk::AttachmentLoadOp::eDontCare;
        depthAttachment.storeOp = vk::AttachmentStoreOp::eStore;
        depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
        depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::AttachmentReference depthAttachmentRef;
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        // 3. Configuração do Subpass
        vk::SubpassDescription subpass;
        subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        // O pulo do gato: Só criamos e passamos o Resolve Attachment se o MSAA estiver ativo
        vk::AttachmentDescription colorAttachmentResolve;
        vk::AttachmentReference colorAttachmentResolveRef;
        std::vector<vk::AttachmentDescription> attachments = { colorAttachment, depthAttachment };

        if (useMSAA) {
            colorAttachmentResolve.format = SwapChainSurfaceFormat.format;
            colorAttachmentResolve.samples = vk::SampleCountFlagBits::e1;
            colorAttachmentResolve.loadOp = vk::AttachmentLoadOp::eDontCare;
            colorAttachmentResolve.storeOp = vk::AttachmentStoreOp::eStore;
            colorAttachmentResolve.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            colorAttachmentResolve.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            colorAttachmentResolve.initialLayout = vk::ImageLayout::eUndefined;
            colorAttachmentResolve.finalLayout = vk::ImageLayout::ePresentSrcKHR;

            colorAttachmentResolveRef.attachment = 2;
            colorAttachmentResolveRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

            subpass.pResolveAttachments = &colorAttachmentResolveRef;
            attachments.push_back(colorAttachmentResolve); // Array com 3 elementos
        }
        else {
            subpass.pResolveAttachments = nullptr; // Sem resolve para 1-sample
        }

        vk::SubpassDependency dependency;
        dependency.srcSubpass = vk::SubpassExternal;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
        dependency.srcAccessMask = vk::AccessFlagBits::eNone;
        dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
        dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

        vk::RenderPassCreateInfo renderPassInfo;
        renderPassInfo.sType = vk::StructureType::eRenderPassCreateInfo;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        RenderPass = vk::raii::RenderPass(Device, renderPassInfo);
    }

    void CreateFramebuffers() {
        SwapChainFramebuffers.clear();
        SwapChainFramebuffers.reserve(SwapChainImageViews.size());

        bool useMSAA = (MSAASamples != vk::SampleCountFlagBits::e1);

        for (size_t i = 0; i < SwapChainImageViews.size(); i++) {
            std::vector<vk::ImageView> attachments;

            if (useMSAA) {
                // Ordem com MSAA: [0]=ColorBuffer (MSAA), [1]=DepthBuffer (MSAA), [2]=Swapchain (Resolve)
                attachments = {
                    ColorBuffer->ColorImageView,
                    DepthBuffer->DepthImageView,
                    SwapChainImageViews[i]
                };
            }
            else {
                // Ordem sem MSAA: [0]=Swapchain (Direct Color), [1]=DepthBuffer (Standard)
                attachments = {
                    SwapChainImageViews[i],
                    DepthBuffer->DepthImageView
                };
            }

            vk::FramebufferCreateInfo framebufferInfo;
            framebufferInfo.sType = vk::StructureType::eFramebufferCreateInfo;
            framebufferInfo.renderPass = RenderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = SwapChainExtent.width;
            framebufferInfo.height = SwapChainExtent.height;
            framebufferInfo.layers = 1;

            SwapChainFramebuffers.emplace_back(Device, framebufferInfo);
        }
    }

    vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width != 0xFFFFFFFF)
        {
            return capabilities.currentExtent;
        }
        int width, height;
        SDL_GetWindowSizeInPixels(Window, &width, &height);

        return {
            std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
        };
    }

    static vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats)
    {
        assert(!availableFormats.empty());
        const auto formatIt = std::ranges::find_if(
            availableFormats,
            [](const auto& format)
            {
                return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace ==
                    vk::ColorSpaceKHR::eSrgbNonlinear;
            });
        return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
    }

    static uint32_t ChooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities)
    {
        auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
        if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount))
        {
            minImageCount = surfaceCapabilities.maxImageCount;
        }
        return minImageCount;
    }

    vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
    {
        assert(std::ranges::any_of(
            availablePresentModes,
            [](auto presentMode)
            {
                return presentMode == vk::PresentModeKHR::eFifo;
            }));

        switch (GUserSettings->Vsync)
        {
        case Immediate:
            return vk::PresentModeKHR::eImmediate;
        case Adaptative:
            return vk::PresentModeKHR::eFifo;
        case TripleBuffering:
            return std::ranges::any_of(
                availablePresentModes,
                [this](const vk::PresentModeKHR value)
                {
                    return value == vk::PresentModeKHR::eMailbox;
                })
                ? vk::PresentModeKHR::eMailbox
                : vk::PresentModeKHR::eFifo;
        }

        return vk::PresentModeKHR::eFifo;
    }

    void CreateImageViews()
    {
        assert(SwapChainImageViews.empty());

        SwapChainImageViews.reserve(SwapChainImages.size());
        for (auto& image : SwapChainImages)
        {
            SwapChainImageViews.emplace_back(VKUtils::CreateImageView(image, SwapChainSurfaceFormat.format, vk::ImageAspectFlagBits::eColor, 1));
        }
    }

    void SyncUserSettings()
    {
        if ((uint8_t)GUserSettings->MSAASamples != (uint8_t)MSAASamples)
        {
            MSAASamples = (vk::SampleCountFlagBits)GUserSettings->MSAASamples;
            if (MSAASamples > MaxMSAASamples)
            {
                MSAASamples = MaxMSAASamples;
                GUserSettings->MSAASamples = (EMSAASample)MaxMSAASamples;
            }
        }
    }

    void RecreateSwapChain()
    {
        int width = 0, height = 0;
        while (width < 1 || height < 1)
        {
            if (!SDL_GetWindowSizeInPixels(Window, &width, &height))
            {
                Logs::SdlError();
            }
            SDL_WaitEvent(NULL);
        }

        Device.waitIdle();

        SyncUserSettings();
        CleanupSwapChain();
        CreateSwapChain();
        CreateImageViews();
        ColorBuffer->Init(MSAASamples);
        DepthBuffer->Init(SwapChainExtent, MSAASamples);

        CreateFramebuffers();
    }

    void RecordCommandBuffer(uint32_t imageIndex)
    {
        auto& commandBuffer = CommandBuffers[FrameIndex];
        commandBuffer.begin({});

        // Se MSAA estiver ativo, renderizamos no ColorBuffer e resolvemos no Swapchain.
        // Se for e1, renderizamos DIRETO no Swapchain.
        bool useMSAA = (MSAASamples != vk::SampleCountFlagBits::e1);

        if (useMSAA)
        {
            // 1. Transita o buffer multi-sampled (MSAA)
            TransitionImageLayout(
                *ColorBuffer->ColorImage,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eColorAttachmentOptimal,
                {},
                vk::AccessFlagBits2::eColorAttachmentWrite,
                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                vk::ImageAspectFlagBits::eColor
            );
        }

        // 2. Transita a imagem que receberá a renderização final/direta (Swapchain)
        TransitionImageLayout(
            SwapChainImages[imageIndex],
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            {},
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::ImageAspectFlagBits::eColor
        );

        // 3. Transita o Depth Buffer
        TransitionImageLayout(
            *DepthBuffer->DepthImage,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthAttachmentOptimal,
            vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
            vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
            vk::ImageAspectFlagBits::eDepth
        );

        vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
        vk::RenderingAttachmentInfo colorAttachmentInfo;

        if (useMSAA)
        {
            // Com MSAA: Alvo principal é o ColorBuffer, Swapchain é o resolve
            colorAttachmentInfo.imageView = ColorBuffer->ColorImageView;
            colorAttachmentInfo.resolveImageView = SwapChainImageViews[imageIndex];
            colorAttachmentInfo.resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal;
            colorAttachmentInfo.resolveMode = vk::ResolveModeFlagBits::eAverage;
        }
        else
        {
            // Sem MSAA (e1): Alvo principal é direto o Swapchain, sem resolve
            colorAttachmentInfo.imageView = SwapChainImageViews[imageIndex];
            colorAttachmentInfo.resolveImageView = nullptr;
            colorAttachmentInfo.resolveMode = vk::ResolveModeFlagBits::eNone;
        }

        colorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
        colorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
        colorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
        colorAttachmentInfo.clearValue = clearColor;

        vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);
        vk::RenderingAttachmentInfo depthAttachmentInfo;
        depthAttachmentInfo.imageView = DepthBuffer->DepthImageView;
        depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
        depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
        depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
        depthAttachmentInfo.clearValue = clearDepth;

        vk::Rect2D renderArea;
        renderArea.offset.x = 0;
        renderArea.offset.y = 0;
        renderArea.extent = SwapChainExtent;

        vk::RenderingInfo renderingInfo;
        renderingInfo.renderArea = renderArea;
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachmentInfo;
        renderingInfo.pDepthAttachment = &depthAttachmentInfo;

        commandBuffer.beginRendering(renderingInfo);

        if (OnRender) OnRender(FrameIndex);

        commandBuffer.endRendering();

        // 4. Transita a imagem do Swapchain de ColorAttachment para Present
        TransitionImageLayout(
            SwapChainImages[imageIndex],
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            {},
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eBottomOfPipe,
            vk::ImageAspectFlagBits::eColor
        );

        commandBuffer.end();
    }

    void TransitionImageLayout(
        vk::Image image,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::AccessFlags2 srcAccessMask,
        vk::AccessFlags2 dstAccessMask,
        vk::PipelineStageFlags2 srcStageMask,
        vk::PipelineStageFlags2 dstStageMask,
        vk::ImageAspectFlags imageAspectFlags
    )
    {
        vk::ImageSubresourceRange sourceRange;
        sourceRange.aspectMask = imageAspectFlags;
        sourceRange.baseMipLevel = 0;
        sourceRange.levelCount = 1;
        sourceRange.baseArrayLayer = 0;
        sourceRange.layerCount = 1;

        vk::ImageMemoryBarrier2 barrier;
        barrier.srcStageMask = srcStageMask;
        barrier.srcAccessMask = srcAccessMask;
        barrier.dstStageMask = dstStageMask;
        barrier.dstAccessMask = dstAccessMask;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange = sourceRange;

        vk::DependencyInfo dependencyInfo;
        dependencyInfo.dependencyFlags = {};
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &barrier;

        CommandBuffers[FrameIndex].pipelineBarrier2(dependencyInfo);
    }
};
