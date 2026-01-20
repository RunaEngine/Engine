#pragma once

#include <SDL3/SDL.h>
#include <imgui.h>
#include <string>
#include <functional>

namespace runa::runtime::opengl {
    enum EDriver : uint8_t {
        core = 0,
        es = 1,
    };

    class Backend
    {
    public:
        Backend() = default;
        ~Backend();

        bool init(EDriver driver);
        void deinit();
        
        SDL_Window* getWindow() const;
        SDL_GLContext getContext() const;
        const char* getGlslVersion();
    private:
        SDL_Window* windowPtr = nullptr;
        SDL_GLContext context = nullptr;
        const char* glslVersion = "";
    };

    class ImGuiBackend
    {
    public:
        ImGuiBackend() = default;
        ~ImGuiBackend();

        void init(Backend& backend);
        void deinit();

        bool isInitialized() const { return initialized; }

        ImGuiIO* getIO();
    private:
        bool initialized = false;
        ImGuiIO* io = nullptr;
    };

    class Render {
    public:
        Render() = default;
        ~Render();

        bool init(EDriver driver = core, bool useImgui = true);
        void deinit();

        void poll();

        const Backend& getBackend() { return backend; }
        const ImGuiBackend& getImGuiBackend() { return imguiBackend; }

        std::function<void(SDL_Event&)> onEvent;
        std::function<void(ImGuiIO&)> onImGuiRender;
        std::function<void(double)> onRender;
    private:
        bool initialized = false;
        Backend backend;
        ImGuiBackend imguiBackend;
    };
}
