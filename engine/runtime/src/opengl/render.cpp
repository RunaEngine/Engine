#include "opengl/render.h"
#include "runtime.h"
#include "utils/logs.h"
#include "settings.h"
#include "input.h"
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <glad/glad.h>

namespace runa::runtime::opengl {
    Backend::~Backend()
    {
        deinit();
    }

    bool Backend::init(EDriver driver)
    {
        /* Init SDL Video */
        if (SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) {
            utils::Logs::error("SDL video already initialized");
            return false;
        }

        if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
        {
            utils::Logs::sdlError();
            return false;
        }

        // Configure driver
        switch (driver) {
        case es:
            if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES))
            {
                utils::Logs::sdlError();
                return false;
            }
            if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3))
            {
                utils::Logs::sdlError();
                return false;
            }
            if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2))
            {
                utils::Logs::sdlError();
                return false;
            }
            glslVersion = "#version 320 es";
            break;
        case core:
            if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE))
            {
                utils::Logs::sdlError();
                return false;
            }
            if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4))
            {
                utils::Logs::sdlError();
                return false;
            }
            if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6))
            {
                utils::Logs::sdlError();
                return false;
            }
            glslVersion = "#version 460";
            break;
        default:
            SDL_Quit();
            return 1;
        }

        // Create a SDL window
        windowPtr = SDL_CreateWindow("Runa", 1024, 576, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
        if (!windowPtr) {
            utils::Logs::sdlError();
            SDL_Quit();
            return false;
        }

        // Create a SDL renderer
       context = SDL_GL_CreateContext(windowPtr);
        if (!context) {
            SDL_DestroyWindow(windowPtr);
            utils::Logs::sdlError();
            SDL_Quit();
            return false;
        }

        if (driver == es) {
            if (!gladLoadGLES2Loader((GLADloadproc)SDL_GL_GetProcAddress)) {
                SDL_DestroyWindow(windowPtr);
                SDL_GL_DestroyContext(context);
                SDL_Quit();
                utils::Logs::sdlError();
                return false;
            }
        }
        else {
            if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
                SDL_DestroyWindow(windowPtr);
                SDL_GL_DestroyContext(context);
                SDL_Quit();
                utils::Logs::sdlError();
                return false;
            }
        }

        glEnable(GL_DEPTH_TEST);

        return true;
    }

    void Backend::deinit()
    {
        if (context)
            SDL_GL_DestroyContext(context);
        context = nullptr;
        if (windowPtr)
            SDL_DestroyWindow(windowPtr);
        windowPtr = nullptr;
        glslVersion  = "";
        if (SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)
        {
            SDL_Quit();
        }
    }

    SDL_Window* Backend::getWindow() const
    {
        return windowPtr;
    }

    SDL_GLContext Backend::getContext() const
    {
        return context;
    }

    const char* Backend::getGlslVersion()
    {
        return glslVersion;
    }

    ImGuiBackend::~ImGuiBackend()
    {
        if (initialized) deinit();
    }

    void ImGuiBackend::init(Backend& backend)
    {
        if (!backend.getWindow() || !backend.getContext())
            return;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        io = &ImGui::GetIO();
        ImGui_ImplSDL3_InitForOpenGL(backend.getWindow(), backend.getContext());
        ImGui_ImplOpenGL3_Init(backend.getGlslVersion());
        initialized = true;
    }

    void ImGuiBackend::deinit()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        initialized = false;
        io = nullptr;
    }

    ImGuiIO* ImGuiBackend::getIO()
    {
        return io;
    }

    Render::~Render()
    {

    }

    bool Render::init(EDriver driver, bool useImgui) {
        if (!backend.init(driver)) return false;
        if (useImgui) imguiBackend.init(backend);

        return true;
    }

    void Render::deinit() {
        imguiBackend.deinit();
        backend.deinit();
    }

    void Render::poll() {
        bool should_limit = gameUserSettings.getFramerateLimit() > 0 && gameUserSettings.getVsync() == disable;
        uint64_t frame_time = 0;
        if (should_limit) {
            frame_time = 1000000000 / gameUserSettings.getFramerateLimit();
        }

        // Render imgui
        if (imguiBackend.isInitialized()) {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();
        }

        // Render behind imgui
        glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (onRender) onRender(tick.delta());

        if (imguiBackend.isInitialized()) {
            if (onImGuiRender) onImGuiRender(ImGui::GetIO());
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        SDL_GL_SwapWindow(backend.getWindow());
        // Finish render
        if (should_limit) {
            if (frame_time > 0 && frame_time > tick.elapsedNS()) {
                SDL_DelayPrecise(frame_time - tick.elapsedNS());
            }
        }
    }
}

