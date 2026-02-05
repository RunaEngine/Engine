#include "opengl/mesh.h"
#include "utils/logs.h"

namespace runa::runtime::opengl
{
    Mesh::~Mesh()
    {
        if (deferDeinit)
            deinit();
    }

    bool Mesh::init(std::vector<Vertex>& vertices, std::vector<GLuint>& indices, std::vector<Texture>& textures)
    {
        this->textures = textures;

        vao.init();
        vao.bind();
        // Generates Vertex Buffer Object and links it to vertices
        vbo.init(vertices);
        vbo.defer(false);
        // Generates Element Buffer Object and links it to indices
        ebo.init(indices);
        ebo.defer(false);
        // Links VBO attributes such as coordinates and colors to VAO
        GLsizei stride = sizeof(Vertex);
        vao.enableAttrib(vbo, 0, 3, GL_FLOAT, stride, (void*)0);
        vao.enableAttrib(vbo, 1, 3, GL_FLOAT, stride, (void*)offsetof(Vertex, normal));
        vao.enableAttrib(vbo, 2, 3, GL_FLOAT, stride, (void*)offsetof(Vertex, color));
        vao.enableAttrib(vbo, 3, 2, GL_FLOAT, stride, (void*)offsetof(Vertex, texUV));
        // Unbind all to prevent accidentally modifying them
        vao.unbind();
        vbo.unbind();
        ebo.unbind();

        return true;
    }

    bool Mesh::init(std::vector<Vertex>& vertices, std::vector<GLuint>& indices)
    {
        vao.init();
        vao.defer(false);
        vao.bind();
        // Generates Vertex Buffer Object and links it to vertices
        vbo.init(vertices);
        vbo.defer(false);
        // Generates Element Buffer Object and links it to indices
        ebo.init(indices);
        ebo.defer(false);
        // Links VBO attributes such as coordinates and colors to VAO
        GLsizei stride = sizeof(Vertex);
        (void*)offsetof(Vertex, normal);
        vao.enableAttrib(vbo, 0, 3, GL_FLOAT, stride, (void*)0);
        vao.enableAttrib(vbo, 1, 3, GL_FLOAT, stride, (void*)offsetof(Vertex, normal));
        vao.enableAttrib(vbo, 2, 3, GL_FLOAT, stride, (void*)offsetof(Vertex, color));
        vao.enableAttrib(vbo, 3, 2, GL_FLOAT, stride, (void*)offsetof(Vertex, texUV));
        // Unbind all to prevent accidentally modifying them
        vao.unbind();
        vbo.unbind();
        ebo.unbind();

        return true;
    }

    void Mesh::deinit()
    {
        vao.deinit();
        vbo.deinit();
        ebo.deinit();
        for (Texture& t : textures)
        {
            t.denit();
        }
        textures.clear();
    }

    void Mesh::defer(bool value)
    {
        deferDeinit = value;
    }

    void Mesh::draw(const Shader& shader, const Camera& camera, 
            glm::mat4 matrix,
            glm::vec3 position,
            glm::quat rotation,
            glm::vec3 scale
        )
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
                if (SDL_snprintf(uniform, sizeof(uniform), "%s%u", type, numDiffuse) < 0)
                {
                    utils::Logs::sdlError();
                    continue;
                }
                numDiffuse++;
            }
            else if (SDL_strcmp(type, "specular") == 0)
            {
                if (SDL_snprintf(uniform, sizeof(uniform), "%s%u", type, numSpecular) < 0)
                {
                    utils::Logs::sdlError();
                    continue;
                }
                numSpecular++;
            }
            textures[i].texUnit(shader, uniform, i);
            textures[i].bind();
        }
        // Take care of the camera Matrix
        glUniform3f(glGetUniformLocation(shader.getID(), "camPos"), camera.position.x, camera.position.y, camera.position.z);
        camera.matrix(shader, "camMatrix");

        // Initialize matrices
        glm::mat4 pos = glm::mat4(1.0f);
        glm::mat4 rot = glm::mat4(1.0f);
        glm::mat4 sca = glm::mat4(1.0f);

        // Transform the matrices to their correct form
        pos = glm::translate(pos, position);
        rot = glm::mat4_cast(rotation);
        sca = glm::scale(sca, scale);

        // Push the matrices to the vertex shader
        glUniformMatrix4fv(glGetUniformLocation(shader.getID(), "translation"), 1, GL_FALSE, glm::value_ptr(pos));
        glUniformMatrix4fv(glGetUniformLocation(shader.getID(), "rotation"), 1, GL_FALSE, glm::value_ptr(rot));
        glUniformMatrix4fv(glGetUniformLocation(shader.getID(), "scale"), 1, GL_FALSE, glm::value_ptr(sca));
        glUniformMatrix4fv(glGetUniformLocation(shader.getID(), "model"), 1, GL_FALSE, glm::value_ptr(matrix));

        // Draw the actual mesh
        glDrawElements(GL_TRIANGLES, ebo.count(), GL_UNSIGNED_INT, 0);
    }
}
