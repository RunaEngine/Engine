#include "opengl/camera.h"
#include "opengl/render.h"
#include "input.h"


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
    }

    void camera_c::tick(float delta) {
        direction = glm::clamp(direction, glm::vec3(-1.0f), glm::vec3(1.0f));
        pos += speed * direction * delta;
    }
}
