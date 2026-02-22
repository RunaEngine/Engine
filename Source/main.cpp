#include <iostream>
#include "Vulkan/Pipeline.h"
#include "Io/Event.h"

int main(int argc, char** argv)
{
    Pipeline pipeline;
    Event event;
    if (!pipeline.Init())
        return -1;

    bool shouldClose = false;
    event.OnEvent = [&](SDL_Event &e) {
        if (e.type == SDL_EVENT_QUIT)
        {
            shouldClose = true;
        }
    };

    while (!shouldClose)
    {
        event.Run(EPool);
        pipeline.Pool();
    }

    return 0;
}