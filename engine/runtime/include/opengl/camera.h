#include "shader.h"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/vector_angle.hpp>


namespace runa::runtime::opengl {
    class Camera {
    public:
        Camera() = default;
        ~Camera();

        // Camera main vectors
        glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 rotation = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 orientation = glm::vec3(0.0f, 0.0f, -1.0f);

        // Camera speed
        float speed = 4.0f;
        float sensitivity = 120.0f;

        // Updates and exports the camera matrix to the Vertex Shader
        void updateMatrix(float FOVdeg, float nearPlane, float farPlane);
        void matrix(const Shader &shader, const char *uniform) const;
        // Handles camera inputs
        void inputs(SDL_Event &event);
        void tick(float delta);

    private:
        glm::mat4 cmatrix = glm::identity<glm::mat4>();
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

        // Window w/h
        int width = 0;
        int height = 0;
    };
}
