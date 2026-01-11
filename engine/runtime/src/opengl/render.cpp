#include "opengl/render.h"
#include "settings.h"
#include "input.h"
#include "timer.h"
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <glad/glad.h>

namespace runa::runtime {
    int init_opengl(backend_t& backend, driver_e driver) {
        /* Init SDL Video */
        if (!SDL_WasInit(SDL_INIT_VIDEO)) {
            if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
                return SDL_INIT_VIDEO;
        }

        // Configure driver
        switch (driver) {
        case opengl_es:
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
            backend.glsl_version = "#version 320 es";
            break;
        case opengl_core:
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
            backend.glsl_version = "#version 460";
            break;
        default:
            SDL_Quit();
            return 1;
        }

        // Create a SDL window
        backend.window_ptr = SDL_CreateWindow("Runa", 1024, 576, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
        if (!backend.window_ptr) {
            SDL_Log("Failed to create window");
            return 1;
        }

        // Ensure window borders are visible on Wayland
        SDL_SetWindowBordered(backend.window_ptr, true);

        // Create a SDL renderer
        backend.context = SDL_GL_CreateContext(backend.window_ptr);
        if (!backend.context) {
            SDL_DestroyWindow(backend.window_ptr);
            SDL_Log("Failed to create context");
            return 1;
        }

        if (driver == opengl_es) {
            if (!gladLoadGLES2Loader((GLADloadproc)SDL_GL_GetProcAddress)) {
                SDL_Log("Failed to initialize GLAD");
                SDL_DestroyWindow(backend.window_ptr);
                SDL_GL_DestroyContext(backend.context);
                return 1;
            }
        }
        else {
            if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
                SDL_Log("Failed to initialize GLAD");
                SDL_DestroyWindow(backend.window_ptr);
                SDL_GL_DestroyContext(backend.context);
                return 1;
            }
        }

        return 0;
    }

    void destroy_opengl(backend_t& backend) {
        if (backend.context)
            SDL_GL_DestroyContext(backend.context);
        if (backend.window_ptr)
            SDL_DestroyWindow(backend.window_ptr);
        backend.glsl_version.clear();
    }

    void init_imgui(backend_t& backend) {
        if (!backend.window_ptr || !backend.context)
            return;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        ImGui_ImplSDL3_InitForOpenGL(backend.window_ptr, backend.context);
        ImGui_ImplOpenGL3_Init(backend.glsl_version.c_str());
    }

    void destroy_imgui() {
        ImGui_ImplSDL3_Shutdown();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui::DestroyContext();
    }

    gl_render_c::gl_render_c()
    {
    }

    gl_render_c::~gl_render_c()
    {
    }

    int gl_render_c::init(config_t config) {
        if (is_initialized) return -1;
        int res = init_opengl(backend, config.driver);
        if (res != 0) return res;
        if (config.render_imgui) {
            init_imgui(backend);
        }
        m_config = config;
        glEnable(GL_DEPTH_TEST);
        is_initialized = true;

        return res;
    }

    void gl_render_c::destroy() {
        if (!is_initialized) return;
        if (m_config.render_imgui)
            destroy_imgui();
        destroy_opengl(backend);
    }

    void gl_render_c::poll() {
        Time.update_current_time();
        bool should_limit = GameUserSettings.get_framerate_limit() > 0 && GameUserSettings.get_vsync() == disable;
        uint64_t frame_time = 0;
        if (should_limit) {
            frame_time = 1000000000 / GameUserSettings.get_framerate_limit();
        }
        
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                int w = 0, h = 0;
                SDL_GetWindowSizeInPixels(backend.window_ptr, &w, &h);
                glViewport(0, 0, w, h);
            }
            Input.update_event(event);
            if (event_cb) event_cb(event);
        }

        // Render imgui
        if (m_config.render_imgui) {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();
        }

        // Render behind imgui
        glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (render_cb) render_cb(Time.delta());

        if (m_config.render_imgui) {
            if (imgui_render_cb) imgui_render_cb(ImGui::GetIO());
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        SDL_GL_SwapWindow(backend.window_ptr);
        // Finish render
        if (should_limit) {
            if (frame_time > 0 && frame_time > Time.elapsed_ns()) {
                SDL_DelayPrecise(frame_time - Time.elapsed_ns());
            }
        }
        Time.update_end_time();
    }

    const backend_t& gl_render_c::get_backend() {
        return backend;
    }

    gl_render_c Render = gl_render_c();
}


