#pragma once

#include <SDL3/SDL.h>
#include <imgui.h>
#include <string>
#include <functional>

namespace runa::runtime {
    enum driver_e : uint8_t {
        opengl_core = 0,
        opengl_es = 1,
    };

    struct backend_t {
        SDL_Window* window_ptr;
        SDL_GLContext context;
        std::string glsl_version;
    };

    struct config_t {
        driver_e driver = opengl_core;
        bool render_imgui = true;
    };

    int init_opengl(backend_t& backend, driver_e driver);
    void destroy_opengl(backend_t& backend);

    void init_imgui(backend_t &backend);
    void destroy_imgui();

    class gl_render_c {
    public:
        gl_render_c();
        ~gl_render_c();

        int init(config_t config = {});
        void destroy();
        void poll();

        const backend_t &get_backend();

        std::function<void(SDL_Event&)> event_cb;
        std::function<void(ImGuiIO&)> imgui_render_cb;
        std::function<void(double)> render_cb;
    private:
        config_t m_config;
        bool is_initialized = false;
        bool window_should_close = false;
        backend_t backend;
    };

    extern gl_render_c Render;
}
