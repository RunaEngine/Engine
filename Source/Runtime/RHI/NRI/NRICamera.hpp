#pragma once
#include "Engine/Core/Object.hpp"
#include "Runtime/Input.hpp"
#include "Runtime/RHI/NRI/NRIBuffer.hpp"
#include <NRI.h>
#include <Extensions/NRIStreamer.h>
#include <Extensions/NRIHelper.h>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/vector_angle.hpp>

struct alignas(256) CameraData
{
    glm::mat4 ViewProj;
};

class NRICamera : public Object
{
private:
    nri::CoreInterface& ICore;
    nri::Device* Device = nullptr;
    SDL_Window* Window = nullptr;

    NRIBuffer CameraBuffer;
    void* CameraBufferMemory = nullptr;

    glm::mat4 ViewMatrix = glm::identity<glm::mat4>();
    glm::mat4 ProjMatrix = glm::identity<glm::mat4>();
    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
    int Width = 0, Height = 0;

public:
    nri::Descriptor* CameraBufferDescriptor = nullptr;
    nri::BufferView* CameraBufferView = nullptr;
    nri::PipelineLayout* PipelineLayout = nullptr;
    nri::DescriptorPool* DescriptorPool = nullptr;
    nri::DescriptorSet* CameraDescriptorSet = nullptr;


    glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 Rotation = glm::vec3(0.0f);
    glm::vec3 Orientation = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));

    float FovDeg = 60.f;
    float NearPlane = 0.1f;
    float FarPlane = 500.f;
    float Speed = 4.0f;
    float Sensitivity = 120.0f;

    NRICamera(nri::CoreInterface& core, nri::Device* device, SDL_Window* window) : ICore(core), Device(device),
        Window(window), CameraBuffer(core, device)
    {
    }

    ~NRICamera() override { Deinit(); }

    bool Init()
    {
        uint64_t alignedSize = NRIUtils::GetAlignedSize(sizeof(glm::mat4), 256);

        if (!CameraBuffer.Init(alignedSize, nri::BufferUsageBits::CONSTANT_BUFFER, nri::MemoryLocation::HOST_UPLOAD))
            return false;

        // Create buffer view
        nri::BufferViewDesc viewDesc = {};
        viewDesc.buffer = CameraBuffer.Get();
        viewDesc.type = nri::BufferView::CONSTANT_BUFFER; // Fixed type fields to match current NRI specification
        viewDesc.offset = 0;
        viewDesc.size = alignedSize;

        if (ICore.CreateBufferView(viewDesc, CameraBufferDescriptor) != nri::Result::SUCCESS)
        {
            Logs::Error("NRICamera: Failed to create buffer view");
            return false;
        }

        // Define bindings layout
        nri::DescriptorRangeDesc descriptorRange = {};
        descriptorRange.baseRegisterIndex = 0;
        descriptorRange.descriptorNum = 1;
        descriptorRange.descriptorType = nri::DescriptorType::CONSTANT_BUFFER;
        descriptorRange.shaderStages = nri::StageBits::VERTEX_SHADER;

        nri::DescriptorSetDesc setDesc = {};
        setDesc.registerSpace = 0;
        setDesc.rangeNum = 1;
        setDesc.ranges = &descriptorRange;

        nri::PipelineLayoutDesc pipelineLayoutDesc = {};
        pipelineLayoutDesc.descriptorSetNum = 1;
        pipelineLayoutDesc.descriptorSets = &setDesc;
        pipelineLayoutDesc.shaderStages = nri::StageBits::VERTEX_SHADER;

        if (ICore.CreatePipelineLayout(*Device, pipelineLayoutDesc, PipelineLayout) != nri::Result::SUCCESS)
        {
            Logs::Error("NRICamera: Failed to create pipeline layout");
            return false;
        }

        // Create descriptor pool
        nri::DescriptorPoolDesc poolDesc = {};
        poolDesc.descriptorSetMaxNum = 1;
        poolDesc.constantBufferMaxNum = 1;

        if (ICore.CreateDescriptorPool(*Device, poolDesc, DescriptorPool) != nri::Result::SUCCESS)
        {
            Logs::Error("NRICamera: Failed to create descriptor pool");
            return false;
        }

        // Allocate descriptor set
        if (ICore.AllocateDescriptorSets(*DescriptorPool, *PipelineLayout, 0, &CameraDescriptorSet, 1, 0) != nri::Result::SUCCESS)
        {
            Logs::Error("NRICamera: Failed to allocate descriptor set");
            return false;
        }

        // Update descriptor ranges
        nri::UpdateDescriptorRangeDesc updateRangeDesc = {};
        updateRangeDesc.descriptorSet = CameraDescriptorSet;
        updateRangeDesc.rangeIndex = 0;
        updateRangeDesc.baseDescriptor = 0;
        updateRangeDesc.descriptors = (const nri::Descriptor**)&CameraBufferDescriptor;
        updateRangeDesc.descriptorNum = 1;
        ICore.UpdateDescriptorRanges(&updateRangeDesc, 1);

        CameraBufferMemory = CameraBuffer.Map(0, CameraBuffer.GetCapacity());

        return true;
    }

    void Deinit()
    {
        if (CameraBufferMemory)
        {
            CameraBufferMemory = nullptr;
        }
        CameraBuffer.Deinit();

        if (CameraBufferDescriptor)
        {
            ICore.DestroyDescriptor(CameraBufferDescriptor);
            CameraBufferDescriptor = nullptr;
        }
        if (PipelineLayout)
        {
            ICore.DestroyPipelineLayout(PipelineLayout);
            PipelineLayout = nullptr;
        }
        if (DescriptorPool)
        {
            // Note: Destroying the pool implicitly frees the allocated descriptor sets
            ICore.DestroyDescriptorPool(DescriptorPool);
            DescriptorPool = nullptr;
            CameraDescriptorSet = nullptr;
        }
    }

    void UpdateMatrix()
    {
        if (!SDL_GetWindowSize(Window, &Width, &Height))
        {
            return;
        }

        int h = Height < 1 ? 1 : Height;

        ViewMatrix = glm::lookAt(Position, Position + Orientation, Up);
        ProjMatrix = glm::perspective(glm::radians(FovDeg), static_cast<float>(Width) / h, NearPlane, FarPlane);

        //ProjMatrix[1][1] *= -1;

        CameraData data = {};
        data.ViewProj = ProjMatrix * ViewMatrix;
        std::memcpy(
            CameraBufferMemory,
            &data,
            sizeof(CameraData)
        );
    }

    void Inputs(SDL_Event& event)
    {
        glm::vec2 vec = GInput->InputVector(SDL_SCANCODE_D, SDL_SCANCODE_A, SDL_SCANCODE_W, SDL_SCANCODE_S);
        glm::vec3 forward = glm::normalize(Orientation);
        glm::vec3 right = glm::normalize(glm::cross(forward, Up));
        Rotation = right * vec.x + forward * vec.y;
        Rotation += Up * GInput->InputAxis(SDL_SCANCODE_SPACE, SDL_SCANCODE_LCTRL);
        Speed = GInput->KeyPressed(SDL_SCANCODE_LSHIFT) ? 8.0f : 4.0f;

        if (GInput->MouseButtonPressed(SDL_BUTTON_RIGHT))
        {
            SDL_SetWindowMouseGrab(Window, true);
            SDL_SetWindowRelativeMouseMode(Window, true);
            SDL_HideCursor();

            if (event.type == SDL_EVENT_MOUSE_MOTION)
            {
                float rotX = Sensitivity * (float)event.motion.yrel / Height;
                float rotY = Sensitivity * (float)event.motion.xrel / Width;

                glm::vec3 newOrientation = glm::rotate(Orientation, glm::radians(-rotX),
                                                       glm::normalize(glm::cross(Orientation, Up)));

                if (abs(glm::angle(newOrientation, Up) - glm::radians(90.0f)) <= glm::radians(85.0f))
                    Orientation = newOrientation;

                Orientation = glm::rotate(Orientation, glm::radians(-rotY), Up);
            }
        }
        else
        {
            SDL_SetWindowMouseGrab(Window, false);
            SDL_SetWindowRelativeMouseMode(Window, false);
            SDL_ShowCursor();
        }
    }

    void Tick(float delta)
    {
        Rotation = glm::clamp(Rotation, glm::vec3(-1.0f), glm::vec3(1.0f));
        Position += Speed * Rotation * delta;
    }

    glm::mat4 GetViewMatrix() const { return ViewMatrix; }
    glm::mat4 GetProjectionMatrix() const { return ProjMatrix; }
};
