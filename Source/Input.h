#pragma once

#include <map>
#include <SDL3/SDL.h>
#include <glm/common.hpp>
#include <glm/vec2.hpp>

class Input
{
public:
    Input() = default;

    void UpdateEvent(SDL_Event& event);

    bool MouseButtonPressed(int mouseflag);
    bool KeyPressed(SDL_Scancode scancode);

    glm::vec2 InputVector(SDL_Scancode positiveX, SDL_Scancode negativeX, SDL_Scancode positiveY,
                          SDL_Scancode negativeY);
    float InputAxis(SDL_Scancode positive, SDL_Scancode negative);

private:
    std::map<SDL_Scancode, SDL_KeyboardEvent> Scancodes;
    std::map<int, SDL_MouseButtonEvent> Mouseflags;
};
