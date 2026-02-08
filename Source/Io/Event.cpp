#include "Io/Event.h"
#include "Engine/Engine.h"
#include "imgui_impl_sdl3.h"

void Event::Run(EEventMode mode)
{
    if (mode == EPool)
    {
        while (SDL_PollEvent(&e))
        {
            if (GRender->GetImGuiBackend().IsInitialized())
                ImGui_ImplSDL3_ProcessEvent(&e);
            GInput->UpdateEvent(e);
            if (OnEvent) OnEvent(e);
        }
        return;
    }
    while (SDL_WaitEvent(&e))
    {
        if (GRender->GetImGuiBackend().IsInitialized())
            ImGui_ImplSDL3_ProcessEvent(&e);
        GInput->UpdateEvent(e);
        if (OnEvent) OnEvent(e);
    }
}
