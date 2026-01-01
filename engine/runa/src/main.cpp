//#define STC_CSTR_CORE
#include <iostream>
#include <memory>
#include <opengl/render.h>
#include <opengl/camera.h>
#include <opengl/element_buffer.h>
#include <opengl/vertex_array.h>
#include <opengl/vertex_buffer.h>
#include <opengl/texture.h>
#include <opengl/shader.h>
#include <utils/system/path.h>
#include <utils/system/file.h>
#include <opengl/element_count.h>
#include <settings.h>
#include <timer.h>
#include <io/handlers.h>
#include <io/fs.h>

using namespace runa::runtime;

int main(int argc, char** argv) {
    Render.init();
    GameUserSettings.set_vsync(disable);
    GameUserSettings.set_framerate_limit(300);

    GLfloat vertices[] =
    { //     COORDINATES     /        COLORS      /   TexCoord  //
        -0.5f, 0.0f,  0.5f,     0.83f, 0.70f, 0.44f,	0.0f, 0.0f,
        -0.5f, 0.0f, -0.5f,     0.83f, 0.70f, 0.44f,	5.0f, 0.0f,
        0.5f, 0.0f, -0.5f,      0.83f, 0.70f, 0.44f,	0.0f, 0.0f,
        0.5f, 0.0f,  0.5f,      0.83f, 0.70f, 0.44f,	5.0f, 0.0f,
        0.0f, 0.8f,  0.0f,      0.92f, 0.86f, 0.76f,	2.5f, 5.0f
    };

    // Indices for vertices order
    GLuint indices[] =
    {
        0, 1, 2,
        0, 2, 3,
        0, 1, 4,
        1, 2, 4,
        2, 3, 4,
        3, 0, 4
    };

    std::unique_ptr<shader_c> shader;
    std::unique_ptr<element_buffer_c> EBO;
    std::unique_ptr<vertex_array_c> VAO;
    std::unique_ptr<vertex_buffer_c> VBO;
    std::unique_ptr<texture_c> tex;

    GLuint uniID;
    int viewport_width = 1024;
    int viewport_height = 576;

    camera_c camera = camera_c(viewport_width, viewport_height, glm::vec3(0.0f, 0.0f, 2.0f));

    const std::string currentDir = runa::runtime::current_dir();
    const std::string vert_shader = currentDir + "resources/shaders/default.vert";
    const std::string frag_shader = currentDir + "resources/shaders/default.frag";
    const std::string src = runa::runtime::load_text_file(frag_shader);
    shader = std::make_unique<shader_c>(vert_shader, frag_shader);
    VAO = std::make_unique<vertex_array_c>();
    VAO->bind();
    VBO = std::make_unique<vertex_buffer_c>(vertices, sizeof(vertices));
    EBO = std::make_unique<element_buffer_c>(indices, sizeof(indices));
    VAO->enable_attrib(*VBO, 0, 3, GL_FLOAT, 8 * sizeof(GLfloat), (void*)0);
    VAO->enable_attrib(*VBO, 1, 3, GL_FLOAT, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    VAO->enable_attrib(*VBO, 2, 2, GL_FLOAT, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    VAO->unbind();

    VAO->unbind();
    VBO->unbind();
    EBO->unbind();

    uniID = glGetUniformLocation(shader->get_id(), "scale");
    std::string albedodir = currentDir + "resources/textures/brick.png";
    tex = std::make_unique<texture_c>(albedodir, GL_TEXTURE_2D, GL_TEXTURE0, GL_RGB, GL_UNSIGNED_BYTE);
    shader->set_uniform_location("tex0", 0);

    loop_c loop = loop_c();
    

    bool should_close = false;
    Render.event_cb = [&](SDL_Event &event) {
        if (event.type == SDL_EVENT_WINDOW_RESIZED) {
            SDL_GetWindowSizeInPixels(Render.get_backend().window_ptr, &viewport_width, &viewport_height);
        }
        if (event.type == SDL_EVENT_QUIT)
        {
            should_close = true;
        }
        camera.inputs(event);
    };
    Render.imgui_render_cb = [&](ImGuiIO &io) {
        ImGui::Begin("teste");
        ImGui::Text(std::to_string(Time.delta()).c_str());
        ImGui::End();
    };
    Render.render_cb = [&](double delta) {
        shader->use();

        camera.tick((float)delta);
        camera.matrix(60.0f, 0.1f, 100.0f, *shader, "camMatrix");

        glUniform1f(uniID, 0.5f);
        tex->bind();
        VAO->bind();
        glDrawElements(GL_TRIANGLES, GL_ELEMENT_COUNT, GL_UNSIGNED_INT, 0);
    };
    
    while (!should_close) 
    {
        loop.run(UV_RUN_NOWAIT);
        Render.poll();
    }

    Render.destroy();
    /*
    if (code != 0) {
        SDL_Log("%s", SDL_GetError());
        return code;
    }*/

    return 0;
}
