#include "Opengl/Shader.h"
#include "Engine/Core/Object.h"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/vector_angle.hpp>

class Camera : public Object
{
public:
    Camera() = default;
    ~Camera() override;

    // Camera main vectors
    glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 Rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 Orientantion = glm::vec3(0.0f, 0.0f, -1.0f);

    // Camera speed
    float Speed = 4.0f;
    float Sensitivity = 120.0f;

    // Updates and exports the camera matrix to the Vertex Shader
    void UpdateMatrix(float fovdeg, float nearPlane, float farPlane);
    void Matrix(const Shader& shader, const char* uniform) const;
    // Handles camera inputs
    void Inputs(SDL_Event& event);
    void Tick(float Delta);

private:
    glm::mat4 CMatrix = glm::identity<glm::mat4>();
    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);

    // Window w/h
    int Width = 0;
    int Height = 0;
};
