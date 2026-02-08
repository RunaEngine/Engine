#include "Opengl/Camera.h"
#include "Engine/Engine.h"
#include<glm/gtx/vector_angle.hpp>


Camera::~Camera()
{
}

void Camera::UpdateMatrix(float fovdeg, float nearPlane, float farPlane)
{
    if (!SDL_GetWindowSize(GRender->
        GetBackend().GetWindow(), &Width, &Height))
    {
        return;
    }
    // Initializes matrices since otherwise they will be the null matrix
    glm::mat4 view = glm::identity<glm::mat4>();
    glm::mat4 projection = glm::identity<glm::mat4>();

    // Makes camera look in the right direction from the right position
    view = glm::lookAt(Position, Position + Orientantion, Up);
    // Adds perspective to the scene
    projection = glm::perspective(glm::radians(fovdeg), (float)Width / Height, nearPlane, farPlane);

    CMatrix = projection * view;
}

void Camera::Matrix(const Shader& shader, const char* uniform) const
{
    // Exports the camera matrix to the Vertex Shader
    glUniformMatrix4fv(glGetUniformLocation(shader.GetId(), uniform), 1, GL_FALSE, glm::value_ptr(CMatrix));
}

void Camera::Inputs(SDL_Event& event)
{
    glm::vec2 vec = GInput->InputVector(SDL_SCANCODE_D, SDL_SCANCODE_A, SDL_SCANCODE_W, SDL_SCANCODE_S);
    Rotation = glm::normalize(glm::cross(Orientantion, Up)) * vec.x + glm::normalize(Orientantion) * vec.y;

    float y_axis = GInput->InputAxis(SDL_SCANCODE_SPACE, SDL_SCANCODE_LCTRL);
    Rotation.y = y_axis;
    Speed = GInput->KeyPressed(SDL_SCANCODE_LSHIFT) ? 8.0f : 4.0f;

    if (GInput->MouseButtonPressed(SDL_BUTTON_RIGHT))
    {
        SDL_SetWindowMouseGrab(GRender->GetBackend().GetWindow(), true);
        SDL_SetWindowRelativeMouseMode(GRender->GetBackend().GetWindow(), true);
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
        SDL_SetWindowMouseGrab(GRender->GetBackend().GetWindow(), false);
        SDL_SetWindowRelativeMouseMode(GRender->GetBackend().GetWindow(), false);
        SDL_ShowCursor();
    }
}

void Camera::Tick(float Delta)
{
    Rotation = glm::clamp(Rotation, glm::vec3(-1.0f), glm::vec3(1.0f));
    Position += Speed * Rotation * Delta;
}
