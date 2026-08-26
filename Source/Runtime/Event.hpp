#pragma once

#include "Engine/Core/Object.hpp"
#include <SDL3/SDL.h>
#include <imgui_impl_sdl3.h>
#include <functional>

enum EEventMode : uint8_t
{
    EPool = 0,
    EWait = 1,
};

class Event : public Object
{
public:
    EEventMode EventMode = EPool;
    SDL_Event e;

    Event() = default;

    void Run(bool processImguiEvent = false)
    {
        if (EventMode == EPool)
        {
            while (SDL_PollEvent(&e))
            {
                if (processImguiEvent)
                    ImGui_ImplSDL3_ProcessEvent(&e);
                if (OnEvent) OnEvent(e);
            }
            return;
        }
        while (SDL_WaitEvent(&e))
        {
            if (processImguiEvent)
                ImGui_ImplSDL3_ProcessEvent(&e);
            if (OnEvent) OnEvent(e);
        }
    }

    std::function<void(SDL_Event&)> OnEvent;
};

inline SharedPtr<Event> GEvent = MakeShared<Event>();