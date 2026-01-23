//#define STC_CSTR_CORE
#include <iostream>
#include <memory>
#include <runtime.h>
#include <opengl/mesh.h>
#include <utils/system.h>
#include <settings.h>
#include <io/handlers.h>

using namespace runa::runtime;
using namespace runa::runtime::opengl;

int main(int argc, char** argv) {
    if (!render.init()) return -1;
    gameUserSettings.setVsync(disable);
    //gameUserSettings.setFramerateLimit(300);

	// Vertices coordinates
    std::vector<Vertex> vertices =
    {
	    Vertex{
		    glm::vec3(-1.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f),
		    glm::vec2(0.0f, 0.0f)
	    },
	    Vertex{
		    glm::vec3(-1.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f),
		    glm::vec2(0.0f, 1.0f)
	    },
	    Vertex{
		    glm::vec3(1.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f),
		    glm::vec2(1.0f, 1.0f)
	    },
	    Vertex{
		    glm::vec3(1.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(1.0f, 0.0f)
	    }
    };

    // Indices for vertices order
    std::vector<GLuint> indices =
    {
	    0, 1, 2,
	    0, 2, 3
    };

    std::vector<Vertex> lightVertices =
    {
	    //	 COORDINATES	 //
	    Vertex{glm::vec3(-0.1f, -0.1f, 0.1f)},
	    Vertex{glm::vec3(-0.1f, -0.1f, -0.1f)},
	    Vertex{glm::vec3(0.1f, -0.1f, -0.1f)},
	    Vertex{glm::vec3(0.1f, -0.1f, 0.1f)},
	    Vertex{glm::vec3(-0.1f, 0.1f, 0.1f)},
	    Vertex{glm::vec3(-0.1f, 0.1f, -0.1f)},
	    Vertex{glm::vec3(0.1f, 0.1f, -0.1f)},
	    Vertex{glm::vec3(0.1f, 0.1f, 0.1f)}
    };

    std::vector<GLuint> lightIndices =
    {
	    0, 1, 2,
	    0, 2, 3,
	    0, 4, 7,
	    0, 7, 3,
	    3, 7, 6,
	    3, 6, 2,
	    2, 6, 5,
	    2, 5, 1,
	    1, 5, 4,
	    1, 4, 0,
	    4, 5, 6,
	    4, 6, 7
    };

    Camera camera = Camera(glm::vec3(0.0f, 0.0f, 2.0f));

    std::string currentDir = utils::baseDir();

	// Texture data
	std::string albedodir = currentDir + "resources/textures/planks.png";
	std::string speculardir = currentDir + "resources/textures/planksSpec.png";
    std::vector<Texture> textures;
    textures.push_back(Texture());
    textures.push_back(Texture());
    if (!textures[0].init(albedodir.c_str(), "diffuse", 0, 0, GL_UNSIGNED_BYTE))
        return -1;
    if(!textures[1].init(speculardir.c_str(), "specular", 1, 0, GL_UNSIGNED_BYTE))
        return -1;

    std::string vertShader = currentDir + "resources/shaders/default.vert";
    std::string fragShader = currentDir + "resources/shaders/default.frag";
    Shader shader;
    if (!shader.init(vertShader.c_str(), fragShader.c_str()))
    {
        return -1;
    }

	// Store mesh data in vectors for the mesh
	// Create floor mesh
	Mesh floor;
	if (!floor.init(vertices, indices, textures))
	{
		return -1;
	}

    // Shader for light cube
    std::string vertLightShader = currentDir + "resources/shaders/light.vert";
    std::string fragLightShader = currentDir + "resources/shaders/light.frag";
    Shader lightShader;
    if (!lightShader.init(vertLightShader.c_str(), fragLightShader.c_str()))
    {
        return -1;
    }

	// Create light mesh
	Mesh light;
	if (!light.init(lightVertices, lightIndices))
	{
		return -1;
	}

    glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec3 lightPos = glm::vec3(0.5f, 0.5f, 0.5f);
    glm::mat4 lightModel = glm::identity<glm::mat4>();
    lightModel = glm::translate(lightModel, lightPos);

    glm::vec3 pyramidPos = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::mat4 pyramidModel = glm::identity<glm::mat4>();
    pyramidModel = glm::translate(pyramidModel, pyramidPos);

    lightShader.use();
    glUniformMatrix4fv(glGetUniformLocation(lightShader.getID(), "model"), 1, GL_FALSE, glm::value_ptr(lightModel));
    glUniform4f(glGetUniformLocation(lightShader.getID(), "lightColor"), lightColor.x, lightColor.y, lightColor.z, lightColor.w);
    shader.use();
    glUniformMatrix4fv(glGetUniformLocation( shader.getID(), "model"), 1, GL_FALSE, glm::value_ptr(pyramidModel));
    glUniform4f(glGetUniformLocation( shader.getID(), "lightColor"), lightColor.x, lightColor.y, lightColor.z, lightColor.w);
    glUniform3f(glGetUniformLocation( shader.getID(), "lightPos"), lightPos.x, lightPos.y, lightPos.z);

    bool shouldClose = false;
    event.onEvent = [&](SDL_Event &e) {
        if (e.type == SDL_EVENT_WINDOW_RESIZED)
        {
            int viewportWidth, viewportHeight;
            if (SDL_GetWindowSizeInPixels(render.getBackend().getWindow(), &viewportWidth, &viewportHeight))
            {
                glViewport(0, 0, viewportWidth, viewportHeight);
            }
        }
        if (e.type == SDL_EVENT_QUIT)
        {
            shouldClose = true;
        }
        camera.inputs(e);
    };
    render.onImGuiRender = [&](ImGuiIO &io) {
        ImGui::Begin("teste");
        ImGui::Text("FPS: %f", 1.0f / io.DeltaTime);
        ImGui::End();
    };
    render.onRender= [&](double delta) {
        shader.use();

        camera.tick((float)delta);
        camera.updateMatrix(60.0f, 0.1f, 100.0f);

    	// Draws different meshes
    	floor.draw(shader, camera);
    	light.draw(lightShader, camera);
    };

    while (!shouldClose)
    {
        tick.updateCurrentTick();
        event.run(io::pool);
        render.poll();
        tick.updateDeltaTime();
    }

    render.deinit();

    return 0;
}
