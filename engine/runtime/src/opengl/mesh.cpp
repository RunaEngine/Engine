#include "opengl/mesh.h"

#include "utils/logs.h"

namespace runa::runtime::opengl
{
    Mesh::~Mesh()
    {
        deinit();
    }

    bool Mesh::init(std::vector<Vertex>& vertices, std::vector<GLuint>& indices, std::vector<Texture>& textures)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;

        vao.init();
        vao.bind();
        // Generates Vertex Buffer Object and links it to vertices
        VertexBuffer vbo;
        vbo.init(vertices.data(), vertices.size());
        // Generates Element Buffer Object and links it to indices
        ElementBuffer ebo;
        ebo.init(indices.data(), indices.size());
        // Links VBO attributes such as coordinates and colors to VAO
        vao.enableAttrib(vbo, 0, 3, GL_FLOAT, sizeof(Vertex), (void*)0);
        vao.enableAttrib(vbo, 1, 3, GL_FLOAT, sizeof(Vertex), (void*)(3 * sizeof(float)));
        vao.enableAttrib(vbo, 2, 3, GL_FLOAT, sizeof(Vertex), (void*)(6 * sizeof(float)));
        vao.enableAttrib(vbo, 3, 2, GL_FLOAT, sizeof(Vertex), (void*)(9 * sizeof(float)));
        // Unbind all to prevent accidentally modifying them
        vao.unbind();
        vbo.unbind();
        ebo.unbind();

        return true;
    }

    bool Mesh::init(std::vector<Vertex>& vertices, std::vector<GLuint>& indices)
    {
        this->vertices = vertices;
        this->indices = indices;

        vao.init();
        vao.bind();
        // Generates Vertex Buffer Object and links it to vertices
        VertexBuffer vbo;
        vbo.init(vertices.data(), vertices.size());
        // Generates Element Buffer Object and links it to indices
        ElementBuffer ebo;
        ebo.init(indices.data(), indices.size());
        // Links VBO attributes such as coordinates and colors to VAO
        vao.enableAttrib(vbo, 0, 3, GL_FLOAT, sizeof(Vertex), (void*)0);
        vao.enableAttrib(vbo, 1, 3, GL_FLOAT, sizeof(Vertex), (void*)(3 * sizeof(float)));
        vao.enableAttrib(vbo, 2, 3, GL_FLOAT, sizeof(Vertex), (void*)(6 * sizeof(float)));
        vao.enableAttrib(vbo, 3, 2, GL_FLOAT, sizeof(Vertex), (void*)(9 * sizeof(float)));
        // Unbind all to prevent accidentally modifying them
        vao.unbind();
        vbo.unbind();
        ebo.unbind();

        return true;
    }

    void Mesh::deinit()
    {
        vao.deinit();
        vertices.clear();
        indices.clear();
        for (Texture& t : textures)
        {
            t.denit();
        }
        textures.clear();
    }

    void Mesh::draw(const Shader& shader, const Camera& camera)
    {
        // Bind shader to be able to access uniforms
        shader.use();
        vao.bind();

        // Keep track of how many of each type of textures we have
        unsigned int numDiffuse = 0;
        unsigned int numSpecular = 0;

        for (unsigned int i = 0; i < textures.size(); i++)
        {
            const char* type = textures[i].getType();
            char uniform[128];
            if (SDL_strcmp(type, "diffuse") == 0)
            {
                numDiffuse++;
                if (SDL_snprintf(uniform, sizeof(uniform), "%s%d", type, numDiffuse) < 0)
                {
                    utils::Logs::sdlError();
                    continue;
                }
            }
            else if (SDL_strcmp(type, "specular") == 0)
            {
                numSpecular++;
                if (SDL_snprintf(uniform, sizeof(uniform), "%s%d", type, numSpecular) < 0)
                {
                    utils::Logs::sdlError();
                    continue;
                }
            }
            textures[i].texUnit(shader, uniform, i);
            textures[i].bind();
        }
        // Take care of the camera Matrix
        glUniform3f(glGetUniformLocation(shader.getID(), "camPos"), camera.pos.x, camera.pos.y, camera.pos.z);
        camera.matrix(shader, "camMatrix");

        // Draw the actual mesh
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    }
}
