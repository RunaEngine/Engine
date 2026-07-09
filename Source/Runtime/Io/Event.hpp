#pragma once

#include "Engine/Core/Object.hpp"
#include <SDL3/SDL.h>
#include <imgui_impl_sdl3.h>
#include <glm/gtc/constants.hpp>
#include <cstdint>
#include <functional>

enum EEventMode : uint8_t
{
    EPool = 0,
    EWait = 1,
};

class Event : public Object
{
public:
    Event() = default;

    void Run(EEventMode mode)
    {
        if (mode == EPool)
        {
            while (SDL_PollEvent(&e))
            {
                //if (GRender->GetImGuiBackend().IsInitialized())
                //ImGui_ImplSDL3_ProcessEvent(&e);
                if (OnEvent) OnEvent(e);
            }
            return;
        }
        while (SDL_WaitEvent(&e))
        {
            //if (GRender->GetImGuiBackend().IsInitialized())
            //ImGui_ImplSDL3_ProcessEvent(&e);
            if (OnEvent) OnEvent(e);
        }
    }

    std::function<void(SDL_Event&)> OnEvent;

private:
    SDL_Event e;
};
