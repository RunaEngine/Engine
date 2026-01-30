#pragma once

#include "opengl/vertex_array.h"
#include "opengl/element_buffer.h"
#include "opengl/camera.h"
#include "opengl/texture.h"

namespace runa::runtime::opengl
{
    class Mesh
    {
    public:
        Mesh() = default;
        ~Mesh();

        bool init(std::vector<Vertex>& vertices, std::vector<GLuint>& indices, std::vector <Texture>& textures);
        bool init(std::vector<Vertex>& vertices, std::vector<GLuint>& indices);
        void deinit();

        void draw(const Shader& shader, const Camera& camera, 
            glm::mat4 matrix = glm::mat4(1.0f),
            glm::vec3 translation = glm::vec3(0.0f, 0.0f, 0.0f),
            glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
            glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f)
        );
    private:
        std::vector <Vertex> vertices;
        std::vector <GLuint> indices;
        std::vector <Texture> textures;
        // Store VAO in public so it can be used in the Draw function
        VertexArray vao;
    };
}