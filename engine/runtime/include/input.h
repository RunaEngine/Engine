#pragma once

#include <map>
#include <SDL3/SDL.h>
#include <glm/common.hpp>
#include <glm/vec2.hpp>

namespace runa::runtime {
    class Input 
    {
    public:
        Input() = default;

        void updateEvent(SDL_Event &event);

        bool mouseButtonPressed(int mouseflag);
        bool keyPressed(SDL_Scancode scancode);

        glm::vec2 inputVector(SDL_Scancode positiveX, SDL_Scancode negativeX, SDL_Scancode positiveY, SDL_Scancode negativeY);
        float inputAxis(SDL_Scancode positive, SDL_Scancode negative);
    private:
        std::map<SDL_Scancode, SDL_KeyboardEvent> scancodes;
        std::map<int, SDL_MouseButtonEvent> mouseflags;
    };
}
