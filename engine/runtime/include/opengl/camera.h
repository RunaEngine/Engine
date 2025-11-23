#include <opengl/shader.h>
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/vector_angle.hpp>

namespace runa::runtime {
    class camera_c {
    public:
        camera_c(int w, int h, glm::vec3 position);
        ~camera_c();

        // Camera main vectors
        glm::vec3 pos;
        glm::vec3 orientation = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 direction = glm::vec3(0.0f, 0.0f, 0.0f);

        // Prevents the camera from jumping around when first clicking left click
        bool firstClick = true;

        // Window w/h
        int width = 0;
        int height = 0;

        // Camera speed
        float speed = 4.0f;
        float sensitivity = 100.0f;

        // Updates and exports the camera matrix to the Vertex Shader
        void matrix(float FOVdeg, float nearPlane, float farPlane, shader_c &shader, const char *uniform);
        // Handles camera inputs
        void inputs(SDL_Event &event);
        void tick(float delta);
    };
}