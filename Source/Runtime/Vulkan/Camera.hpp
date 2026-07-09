#pragma once

#include "Engine/Core/Object.hpp"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/vector_angle.hpp>


class VKCamera : public Object
{
    SDL_Window* Window = nullptr;
    SharedPtr<Input>& GInput;
public:
    glm::vec3 Position = glm::vec3(0.0f, 0.0f, 5.0f);
    glm::vec3 Rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 Orientantion = glm::vec3(0.0f, 0.0f, -1.0f);

    float Speed = 4.0f;
    float Sensitivity = 120.0f;

    VKCamera(SDL_Window* window, SharedPtr<Input>& input) : Window(window), GInput(input) {}

    void UpdateMatrix(float fovdeg, float nearPlane, float farPlane)
    {
        if (!SDL_GetWindowSize(Window, &Width, &Height))
        {
            return;
        }

        int h = Height < 1 ? 1 : Height;

        ViewMatrix = glm::lookAt(Position, Position + Orientantion, Up);
        ProjMatrix = glm::perspective(glm::radians(fovdeg), static_cast<float>(Width) / h, nearPlane, farPlane);

        ProjMatrix[1][1] *= -1;
    }

    glm::mat4 GetViewMatrix() const { return ViewMatrix; }
    glm::mat4 GetProjectionMatrix() const { return ProjMatrix; }

    void Inputs(SDL_Event& event)
    {
        glm::vec2 vec = GInput->InputVector(SDL_SCANCODE_D, SDL_SCANCODE_A, SDL_SCANCODE_W, SDL_SCANCODE_S);
        Rotation = glm::normalize(glm::cross(Orientantion, Up)) * vec.x + glm::normalize(Orientantion) * vec.y;

        float y_axis = GInput->InputAxis(SDL_SCANCODE_SPACE, SDL_SCANCODE_LCTRL);
        Rotation.y = y_axis;
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

                glm::vec3 newOrientation = glm::rotate(Orientantion, glm::radians(-rotX),
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

private:
    glm::mat4 ViewMatrix = glm::identity<glm::mat4>();
    glm::mat4 ProjMatrix = glm::identity<glm::mat4>();

    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);

    int Width = 0;
    int Height = 0;
};
