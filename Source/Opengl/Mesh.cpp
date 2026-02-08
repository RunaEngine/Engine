#include "Opengl/Mesh.h"
#include "Utils/Logs.h"

Mesh::~Mesh()
{
    Deinit();
}

bool Mesh::Init(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices, const std::vector<SharedPtr<Texture>>& textures)
{
    Textures = textures;

    Vao->Init();
    Vao->Bind();
    // Generates Vertex Buffer Object and links it to vertices
    Vbo->Init(vertices);
    // Generates Element Buffer Object and links it to indices
    Ebo->Init(indices);
    // Links VBO attributes such as coordinates and colors to VAO
    GLsizei stride = sizeof(Vertex);
    Vao->EnableAttrib(*Vbo, 0, 3, GL_FLOAT, stride, (void*)offsetof(Vertex, Position));
    Vao->EnableAttrib(*Vbo, 1, 3, GL_FLOAT, stride, (void*)offsetof(Vertex, Normal));
    Vao->EnableAttrib(*Vbo, 2, 3, GL_FLOAT, stride, (void*)offsetof(Vertex, Color));
    Vao->EnableAttrib(*Vbo, 3, 2, GL_FLOAT, stride, (void*)offsetof(Vertex, TexUV));
    // Unbind all to prevent accidentally modifying them
    Vao->Unbind();
    Vbo->Unbind();
    Ebo->Unbind();

    return true;
}

bool Mesh::Init(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices)
{
    //Vao = MakeUniqueObject<VertexArray>();
    Vao->Init();
    Vao->Bind();
    // Generates Vertex Buffer Object and links it to vertices
    Vbo->Init(vertices);
    // Generates Element Buffer Object and links it to indices
    Ebo->Init(indices);
    // Links VBO attributes such as coordinates and colors to VAO
    GLsizei stride = sizeof(Vertex);
    Vao->EnableAttrib(*Vbo, 0, 3, GL_FLOAT, stride, (void*)offsetof(Vertex, Position));
    Vao->EnableAttrib(*Vbo, 1, 3, GL_FLOAT, stride, (void*)offsetof(Vertex, Normal));
    Vao->EnableAttrib(*Vbo, 2, 3, GL_FLOAT, stride, (void*)offsetof(Vertex, Color));
    Vao->EnableAttrib(*Vbo, 3, 2, GL_FLOAT, stride, (void*)offsetof(Vertex, TexUV));
    // Unbind all to prevent accidentally modifying them
    Vao->Unbind();
    Vbo->Unbind();
    Ebo->Unbind();

    return true;
}

void Mesh::Deinit()
{
    for (auto& t : Textures)
    {
        if (t)
            t->Denit();
    }
    Textures.clear();
}

void Mesh::Draw(const Shader& shader, const Camera& camera,
                glm::mat4 matrix,
                glm::vec3 position,
                glm::quat rotation,
                glm::vec3 scale
)
{
    // Bind shader to be able to access uniforms
    shader.Use();
    Vao->Bind();

    // Keep track of how many of each type of textures we have
    unsigned int numDiffuse = 0;
    unsigned int numSpecular = 0;

    for (unsigned int i = 0; i < Textures.size(); i++)
    {
        if (!Textures[i]) continue; // guard against null/shared-empty entries
        const char* type = Textures[i]->GetType();
        char uniform[128];
        if (SDL_strcmp(type, "diffuse") == 0)
        {
            if (SDL_snprintf(uniform, sizeof(uniform), "%s%u", type, numDiffuse) < 0)
            {
                Logs::SdlError();
                continue;
            }
            numDiffuse++;
        }
        else if (SDL_strcmp(type, "specular") == 0)
        {
            if (SDL_snprintf(uniform, sizeof(uniform), "%s%u", type, numSpecular) < 0)
            {
                Logs::SdlError();
                continue;
            }
            numSpecular++;
        }
        Textures[i]->TexUnit(shader, uniform, i);
        Textures[i]->Bind();
    }
    // Take care of the camera Matrix
    glUniform3f(glGetUniformLocation(shader.GetId(), "camPos"), camera.Position.x, camera.Position.y,
                camera.Position.z);
    camera.Matrix(shader, "camMatrix");

    // Initialize matrices
    glm::mat4 pos = glm::mat4(1.0f);
    glm::mat4 rot = glm::mat4(1.0f);
    glm::mat4 sca = glm::mat4(1.0f);

    // Transform the matrices to their correct form
    pos = glm::translate(pos, position);
    rot = glm::mat4_cast(rotation);
    sca = glm::scale(sca, scale);

    // Push the matrices to the vertex shader
    glUniformMatrix4fv(glGetUniformLocation(shader.GetId(), "translation"), 1, GL_FALSE, glm::value_ptr(pos));
    glUniformMatrix4fv(glGetUniformLocation(shader.GetId(), "rotation"), 1, GL_FALSE, glm::value_ptr(rot));
    glUniformMatrix4fv(glGetUniformLocation(shader.GetId(), "scale"), 1, GL_FALSE, glm::value_ptr(sca));
    glUniformMatrix4fv(glGetUniformLocation(shader.GetId(), "model"), 1, GL_FALSE, glm::value_ptr(matrix));

    // Draw the actual mesh
    glDrawElements(GL_TRIANGLES, Ebo->Count(), GL_UNSIGNED_INT, 0);
}
