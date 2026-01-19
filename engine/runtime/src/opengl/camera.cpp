#include "opengl/camera.h"
#include "opengl/render.h"
#include "input.h"
#include<glm/gtx/vector_angle.hpp>
#include <opengl/render.h>


namespace runa::runtime {
    camera_c::camera_c(int w, int h, glm::vec3 position) {
        width = w;
        height = h;
        pos = position;
    }

    camera_c::~camera_c() {

    }

    void camera_c::matrix(float FOVdeg, float near_plane, float far_plane, shader_c& shader, const char* uniform)
    {
        // Initializes matrices since otherwise they will be the null matrix
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);

        // Makes camera look in the right direction from the right position
        view = glm::lookAt(pos, pos + orientation, up);
        // Adds perspective to the scene
        projection = glm::perspective(glm::radians(FOVdeg), (float)width / height, near_plane, far_plane);

        // Exports the camera matrix to the Vertex Shader
        glUniformMatrix4fv(glGetUniformLocation(shader.get_id(), uniform), 1, GL_FALSE, glm::value_ptr(projection * view));
    }

    void camera_c::inputs(SDL_Event& event) {
        glm::vec2 vec = runtime::Input.get_input_vector(SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_A);
        direction = glm::normalize(glm::cross(orientation, up)) * vec.x + glm::normalize(orientation) * vec.y;

        float y_axis = runtime::Input.get_input_axis(SDL_SCANCODE_SPACE, SDL_SCANCODE_LCTRL);
        direction.y = y_axis;
        speed = runtime::Input.is_key_pressed(SDL_SCANCODE_LSHIFT) ? 8.0f : 4.0f;

        if (runtime::Input.is_mouse_button_pressed(SDL_BUTTON_RIGHT))
        {
            SDL_SetWindowMouseGrab(runtime::Render.get_backend().window_ptr, true);
            SDL_SetWindowRelativeMouseMode(runtime::Render.get_backend().window_ptr, true);
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
            SDL_SetWindowMouseGrab(runtime::Render.get_backend().window_ptr, false);
            SDL_SetWindowRelativeMouseMode(runtime::Render.get_backend().window_ptr, false);
            SDL_ShowCursor();
        }
    }

    void camera_c::tick(float delta) {
        direction = glm::clamp(direction, glm::vec3(-1.0f), glm::vec3(1.0f));
        pos += speed * direction * delta;
    }
}
