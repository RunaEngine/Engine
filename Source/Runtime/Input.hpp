#pragma once

#include "Engine/Core/Object.hpp"
#include <SDL3/SDL.h>
#include <glm/common.hpp>
#include <glm/vec2.hpp>
#include <map>

class Input : public Object
{
private:
    std::map<SDL_Scancode, SDL_KeyboardEvent> Scancodes;
    std::map<int, SDL_MouseButtonEvent> Mouseflags;

public:
    Input() = default;
    ~Input() override = default;

    void UpdateEvent(SDL_Event& event)
    {
        if (event.type == SDL_EVENT_KEY_UP || event.type == SDL_EVENT_KEY_DOWN)
        {
            Scancodes[event.key.scancode] = event.key;
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        {
            Mouseflags[event.button.button] = event.button;
        }
    }

    bool MouseButtonPressed(int mouseflag)
    {
        if (Mouseflags.contains(mouseflag))
        {
            return Mouseflags[mouseflag].down;
        }
        return false;
    }

    bool KeyPressed(SDL_Scancode scancode)
    {
        if (Scancodes.contains(scancode))
        {
            return Scancodes[scancode].down;
        }
        return false;
    }

    glm::vec2 InputVector(SDL_Scancode positiveX, SDL_Scancode negativeX, SDL_Scancode positiveY,
                          SDL_Scancode negativeY)
    {
        glm::vec2 vec = glm::vec2(0.0f);

        if (Scancodes.contains(positiveY))
        {
            vec.y += Scancodes[positiveY].down ? 1.0f : 0.0f;
        }
        if (Scancodes.contains(negativeY))
        {
            vec.y += Scancodes[negativeY].down ? -1.0f : 0.0f;
        }
        if (Scancodes.contains(positiveX))
        {
            vec.x += Scancodes[positiveX].down ? 1.0f : 0.0f;
        }
        if (Scancodes.contains(negativeX))
        {
            vec.x += Scancodes[negativeX].down ? -1.0f : 0.0f;
        }
        vec = glm::clamp(vec, glm::vec2(-1.0f), glm::vec2(1.0f));

        return vec;
    }

    float InputAxis(SDL_Scancode positive, SDL_Scancode negative)
    {
        float axis = 0.0f;

        if (Scancodes.contains(positive))
        {
            axis += Scancodes[positive].down ? 1.0f : 0.0f;
        }
        if (Scancodes.contains(negative))
        {
            axis += Scancodes[negative].down ? -1.0f : 0.0f;
        }
        axis = glm::clamp(axis, -1.0f, 1.0f);

        return axis;
    }
};
