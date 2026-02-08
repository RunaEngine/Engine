#include "Opengl/Render.h"
#include "Engine/Engine.h"
#include "Utils/Logs.h"
#include "Settings.h"
#include "Input.h"
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <glad/glad.h>

GLBackend::~GLBackend()
{
    Deinit();
}

bool GLBackend::Init(EGLDriver driver)
{
    /* Init SDL Video */
    if (SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)
    {
        Logs::Error("SDL video already initialized");
        return false;
    }

    if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
    {
        Logs::SdlError();
        return false;
    }

    // Configure driver
    switch (driver)
    {
    case es:
        if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES))
        {
            Logs::SdlError();
            return false;
        }
        if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3))
        {
            Logs::SdlError();
            return false;
        }
        if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2))
        {
            Logs::SdlError();
            return false;
        }
        GlslVersion = "#version 320 es";
        break;
    case core:
        if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE))
        {
            Logs::SdlError();
            return false;
        }
        if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4))
        {
            Logs::SdlError();
            return false;
        }
        if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5))
        {
            Logs::SdlError();
            return false;
        }
        GlslVersion = "#version 460";
        break;
    default:
        SDL_Quit();
        return 1;
    }

    // Create a SDL window
    WindowPtr = SDL_CreateWindow("Runa", 1024, 576, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    if (!WindowPtr)
    {
        Logs::SdlError();
        SDL_Quit();
        return false;
    }

    // Create a SDL renderer
    Context = SDL_GL_CreateContext(WindowPtr);
    if (!Context)
    {
        SDL_DestroyWindow(WindowPtr);
        Logs::SdlError();
        SDL_Quit();
        return false;
    }

    if (driver == es)
    {
        if (!gladLoadGLES2Loader((GLADloadproc)SDL_GL_GetProcAddress))
        {
            SDL_DestroyWindow(WindowPtr);
            SDL_GL_DestroyContext(Context);
            SDL_Quit();
            Logs::SdlError();
            return false;
        }
    }
    else
    {
        if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
        {
            SDL_DestroyWindow(WindowPtr);
            SDL_GL_DestroyContext(Context);
            SDL_Quit();
            Logs::SdlError();
            return false;
        }
    }

    glEnable(GL_DEPTH_TEST);

    return true;
}

void GLBackend::Deinit()
{
    if (Context)
        SDL_GL_DestroyContext(Context);
    Context = nullptr;
    if (WindowPtr)
        SDL_DestroyWindow(WindowPtr);
    WindowPtr = nullptr;
    GlslVersion = "";
    if (SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)
    {
        SDL_Quit();
    }
}

SDL_Window* GLBackend::GetWindow() const
{
    return WindowPtr;
}

SDL_GLContext GLBackend::GetContext() const
{
    return Context;
}

const char* GLBackend::GetGlslVersion()
{
    return GlslVersion;
}

GLImGuiBackend::~GLImGuiBackend()
{
    if (Initialized) Deinit();
}

void GLImGuiBackend::Init(GLBackend& Backend)
{
    if (!Backend.GetWindow() || !Backend.GetContext())
        return;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    Io = &ImGui::GetIO();
    ImGui_ImplSDL3_InitForOpenGL(Backend.GetWindow(), Backend.GetContext());
    ImGui_ImplOpenGL3_Init(Backend.GetGlslVersion());
    Initialized = true;
}

void GLImGuiBackend::Deinit()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    Initialized = false;
    Io = nullptr;
}

ImGuiIO* GLImGuiBackend::getIO()
{
    return Io;
}

GLRender::~GLRender()
{
}

bool GLRender::Init(EGLDriver driver, bool useImgui)
{
    if (!Backend.Init(driver)) return false;
    if (useImgui) ImguiBackend.Init(Backend);

    return true;
}

void GLRender::Deinit()
{
    ImguiBackend.Deinit();
    Backend.Deinit();
}

void GLRender::Poll()
{
    bool should_limit = GUserSettings->GetFramerateLimit() > 0 && GUserSettings->GetVsync() == Disable;
    uint64_t frame_time = 0;
    if (should_limit)
    {
        frame_time = 1000000000 / GUserSettings->GetFramerateLimit();
    }

    // Render imgui
    if (ImguiBackend.IsInitialized())
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    // Render behind imgui
    glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (OnRender) OnRender(GTick->Delta());

    if (ImguiBackend.IsInitialized())
    {
        if (OnImGuiRender) OnImGuiRender(ImGui::GetIO());
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    SDL_GL_SwapWindow(Backend.GetWindow());
    // Finish render
    if (should_limit)
    {
        if (frame_time > 0 && frame_time > GTick->ElapsedNS())
        {
            SDL_DelayPrecise(frame_time - GTick->ElapsedNS());
        }
    }
}
