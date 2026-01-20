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

        void draw(const Shader& shader, const Camera& camera);
    private:
        std::vector <Vertex> vertices;
        std::vector <GLuint> indices;
        std::vector <Texture> textures;
        // Store VAO in public so it can be used in the Draw function
        VertexArray vao;
    };
}