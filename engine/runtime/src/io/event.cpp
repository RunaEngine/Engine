#include "io/event.h"
#include "runtime.h"
#include "imgui_impl_sdl3.h"

namespace runa::runtime::io
{
    void Event::run(EventMode mode)
    {
        if (mode == pool)
        {
            while (SDL_PollEvent(&event))
            {
                if (render.getImGuiBackend().isInitialized())
                    ImGui_ImplSDL3_ProcessEvent(&event);
                input.updateEvent(event);
                if (onEvent) onEvent(event);
            }
            return;
        }
        while (SDL_WaitEvent(&event))
        {
            if (render.getImGuiBackend().isInitialized())
                ImGui_ImplSDL3_ProcessEvent(&event);
            input.updateEvent(event);
            if (onEvent) onEvent(event);
        }
    }
}
