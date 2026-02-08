#pragma once

#include "Engine/Core/Object.h"
#include <SDL3/SDL.h>
#include <imgui.h>
#include <string>
#include <functional>

enum EGLDriver : uint8_t
{
    core = 0,
    es = 1,
};

class GLBackend : public Object
{
public:
    GLBackend() = default;
    ~GLBackend();

    bool Init(EGLDriver driver);
    void Deinit();

    SDL_Window* GetWindow() const;
    SDL_GLContext GetContext() const;
    const char* GetGlslVersion();

private:
    SDL_Window* WindowPtr = nullptr;
    SDL_GLContext Context = nullptr;
    const char* GlslVersion = "";
};

class GLImGuiBackend : public Object
{
public:
    GLImGuiBackend() = default;
    ~GLImGuiBackend();

    void Init(GLBackend& Backend);
    void Deinit();

    bool IsInitialized() const { return Initialized; }

    ImGuiIO* getIO();

private:
    bool Initialized = false;
    ImGuiIO* Io = nullptr;
};

class GLRender : public Object
{
public:
    GLRender() = default;
    ~GLRender() override;

    bool Init(EGLDriver driver = core, bool useImgui = true);
    void Deinit();

    void Poll();

    const GLBackend& GetBackend() { return Backend; }
    const GLImGuiBackend& GetImGuiBackend() { return ImguiBackend; }

    std::function<void(SDL_Event&)> OnEvent;
    std::function<void(ImGuiIO&)> OnImGuiRender;
    std::function<void(double)> OnRender;

private:
    bool Initialized = false;
    GLBackend Backend;
    GLImGuiBackend ImguiBackend;
};
