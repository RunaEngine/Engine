#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/RHI/wgpu/WGBuffer.hpp"
#include "Runtime/Input.hpp"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/vector_angle.hpp>


class WGCamera : public Object
{
    WGBuffer CameraBuffer;

    SDL_Window* Window = nullptr;
    SharedPtr<Input> GInput = nullptr;

    glm::mat4 ViewMatrix = glm::identity<glm::mat4>();
    glm::mat4 ProjMatrix = glm::identity<glm::mat4>();

    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);

    int Width = 0;
    int Height = 0;
public:
    WGPUBindGroupLayout CameraBindGroupLayout = nullptr;
    WGPUBindGroup CameraBindGroup = nullptr;

    glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 Rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 Orientantion = glm::vec3(0.0f, 0.0f, -1.0f);

    float Fovdeg = 60.f;
    float NearPlane = 0.1f;
    float FarPlane = 100.f;

    float Speed = 4.0f;
    float Sensitivity = 120.0f;

    WGCamera(WGPUDevice device, WGPUQueue queue, SDL_Window* window, SharedPtr<Input> input) : Window(window), GInput(input), CameraBuffer(device, queue)
    {
        CameraBuffer.Init(sizeof(glm::mat4), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);

        WGPUBindGroupLayoutEntry cameraLayoutEntry = {};
        cameraLayoutEntry.binding = 0;
        cameraLayoutEntry.visibility = WGPUShaderStage_Vertex;
        cameraLayoutEntry.buffer.type = WGPUBufferBindingType_Uniform;
        cameraLayoutEntry.buffer.hasDynamicOffset = false;
        cameraLayoutEntry.buffer.minBindingSize = sizeof(glm::mat4);

        WGPUBindGroupLayoutDescriptor cameraLayoutDesc = {};
        cameraLayoutDesc.entryCount = 1;
        cameraLayoutDesc.entries = &cameraLayoutEntry;

        CameraBindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &cameraLayoutDesc);

        WGPUBindGroupEntry cameraBindGroupEntry = {};
        cameraBindGroupEntry.binding = 0;
        cameraBindGroupEntry.buffer = CameraBuffer.Get();
        cameraBindGroupEntry.offset = 0;
        cameraBindGroupEntry.size = sizeof(glm::mat4);

        WGPUBindGroupDescriptor cameraBindGroupDesc = {};
        cameraBindGroupDesc.layout = CameraBindGroupLayout;
        cameraBindGroupDesc.entryCount = 1;
        cameraBindGroupDesc.entries = &cameraBindGroupEntry;

        CameraBindGroup = wgpuDeviceCreateBindGroup(device, &cameraBindGroupDesc);
    }

    void UpdateMatrix()
    {
        if (!SDL_GetWindowSize(Window, &Width, &Height))
        {
            return;
        }

        int h = Height < 1 ? 1 : Height;

        ViewMatrix = glm::lookAt(Position, Position + Orientantion, Up);
        ProjMatrix = glm::perspective(glm::radians(Fovdeg), static_cast<float>(Width) / h, NearPlane, FarPlane);

        ProjMatrix[1][1] *= -1;

        glm::mat4 viewProj = ProjMatrix * ViewMatrix;
        CameraBuffer.Upload(
            &viewProj,
            sizeof(glm::mat4)
        );
    }

    glm::mat4 GetViewMatrix() const { return ViewMatrix; }
    glm::mat4 GetProjectionMatrix() const { return ProjMatrix; }

    void Inputs(SDL_Event& event)
    {
        glm::vec2 vec = GInput->InputVector(SDL_SCANCODE_D, SDL_SCANCODE_A, SDL_SCANCODE_W, SDL_SCANCODE_S);
        Rotation = glm::normalize(glm::cross(Orientantion, Up)) * vec.x + glm::normalize(Orientantion) * vec.y;

        float y_axis = GInput->InputAxis(SDL_SCANCODE_SPACE, SDL_SCANCODE_LCTRL);
        Rotation.y = -y_axis;
        Speed = GInput->KeyPressed(SDL_SCANCODE_LSHIFT) ? 8.0f : 4.0f;

        if (GInput->MouseButtonPressed(SDL_BUTTON_RIGHT))
        {
            SDL_SetWindowMouseGrab(Window, true);
            SDL_SetWindowRelativeMouseMode(Window, true);
            SDL_HideCursor();

            if (event.type == SDL_EVENT_MOUSE_MOTION)
            {
                int xrel = event.motion.xrel;
                int yrel = event.motion.yrel;

                float rotX = Sensitivity * (float)yrel / Height;
                float rotY = Sensitivity * (float)xrel / Width;

                glm::vec3 newOrientation = glm::rotate(Orientantion, glm::radians(rotX),
                    glm::normalize(glm::cross(Orientantion, Up)));

                if (abs(glm::angle(newOrientation, Up) - glm::radians(90.0f)) <= glm::radians(85.0f))
                {
                    Orientantion = newOrientation;
                }

                Orientantion = glm::rotate(Orientantion, glm::radians(-rotY), Up);
            }
        }
        else
        {
            SDL_SetWindowMouseGrab(Window, false);
            SDL_SetWindowRelativeMouseMode(Window, false);
            SDL_ShowCursor();
        }
    }

    void Tick(float Delta)
    {
        Rotation = glm::clamp(Rotation, glm::vec3(-1.0f), glm::vec3(1.0f));
        Position += Speed * Rotation * Delta;
    }

};