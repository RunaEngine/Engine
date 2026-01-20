#include "opengl/camera.h"
#include "runtime.h"
#include<glm/gtx/vector_angle.hpp>


namespace runa::runtime::opengl {
    Camera::Camera(glm::vec3 position)
    {
        pos = position;
    }

    Camera::~Camera()
    {

    }

    void Camera::updateMatrix(float FOVdeg, float nearPlane, float farPlane)
    {
        if (!SDL_GetWindowSize(render.getBackend().getWindow(), &width, &height))
        {
            return;
        }
        // Initializes matrices since otherwise they will be the null matrix
        glm::mat4 view = glm::identity<glm::mat4>();
        glm::mat4 projection = glm::identity<glm::mat4>();

        // Makes camera look in the right direction from the right position
        view = glm::lookAt(pos, pos + orientation, up);
        // Adds perspective to the scene
        projection = glm::perspective(glm::radians(FOVdeg), (float)width / height, nearPlane, farPlane);

        cameraMatrix = projection * view;
    }

    void Camera::matrix(const Shader& shader, const char* uniform) const
    {
        // Exports the camera matrix to the Vertex Shader
        glUniformMatrix4fv(glGetUniformLocation(shader.getID(), uniform), 1, GL_FALSE, glm::value_ptr(cameraMatrix));
    }

    void Camera::inputs(SDL_Event& event) {
        glm::vec2 vec = input.inputVector(SDL_SCANCODE_D, SDL_SCANCODE_A, SDL_SCANCODE_W, SDL_SCANCODE_S);
        direction = glm::normalize(glm::cross(orientation, up)) * vec.x + glm::normalize(orientation) * vec.y;

        float y_axis = input.inputAxis(SDL_SCANCODE_SPACE, SDL_SCANCODE_LCTRL);
        direction.y = y_axis;
        speed = input.keyPressed(SDL_SCANCODE_LSHIFT) ? 8.0f : 4.0f;

        if (input.mouseButtonPressed(SDL_BUTTON_RIGHT))
        {
            SDL_SetWindowMouseGrab(render.getBackend().getWindow(), true);
            SDL_SetWindowRelativeMouseMode(render.getBackend().getWindow(), true);
            SDL_HideCursor();
            

            if (event.type == SDL_EVENT_MOUSE_MOTION)
            {

                int xrel = event.motion.xrel;
                int yrel = event.motion.yrel;

                float rotX = sensitivity * (float)yrel / height;
                float rotY = sensitivity * (float)xrel / width;

                glm::vec3 newOrientation = glm::rotate(orientation, glm::radians(-rotX), glm::normalize(glm::cross(orientation, up)));

                
                if (abs(glm::angle(newOrientation, up) - glm::radians(90.0f)) <= glm::radians(85.0f))
                {
                    orientation = newOrientation;
                }
                

                orientation = glm::rotate(orientation, glm::radians(-rotY), up);
            }
        }
        else
        {
            SDL_SetWindowMouseGrab(render.getBackend().getWindow(), false);
            SDL_SetWindowRelativeMouseMode(render.getBackend().getWindow(), false);
            SDL_ShowCursor();
        }
    }

    void Camera::tick(float delta) {
        direction = glm::clamp(direction, glm::vec3(-1.0f), glm::vec3(1.0f));
        pos += speed * direction * delta;
    }
}
