#include "input.h"

namespace runa::runtime {
    void Input::updateEvent(SDL_Event &event) {
        if (event.type == SDL_EVENT_KEY_UP || event.type == SDL_EVENT_KEY_DOWN) {
            scancodes[event.key.scancode] = event.key;
        } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            mouseflags[event.button.button] = event.button;
        }
    }

    bool Input::mouseButtonPressed(int mouseflag) {
        if (mouseflags.contains(mouseflag)) {
            return mouseflags[mouseflag].down;
        }
        return false;
    }

    bool Input::keyPressed(SDL_Scancode scancode) {
        if (scancodes.contains(scancode)) {
            return scancodes[scancode].down;
        }
        return false;
    }

    glm::vec2 Input::inputVector(SDL_Scancode positiveX, SDL_Scancode negativeX, SDL_Scancode positiveY, SDL_Scancode negativeY) {
        glm::vec2 vec = glm::vec2(0.0f);

        if (scancodes.contains(positiveY)) {
            vec.y += scancodes[positiveY].down ? 1.0f : 0.0f;
        }
        if (scancodes.contains(negativeY)) {
            vec.y += scancodes[negativeY].down ? -1.0f : 0.0f;
        }
        if (scancodes.contains(positiveX)) {
            vec.x += scancodes[positiveX].down ? 1.0f : 0.0f;
        }
        if (scancodes.contains(negativeX)) {
            vec.x += scancodes[negativeX].down ? -1.0f : 0.0f;
        }
        vec = glm::clamp(vec, glm::vec2(-1.0f), glm::vec2(1.0f));

        return vec;
    }

    float Input::inputAxis(SDL_Scancode positive, SDL_Scancode negative) {
        float axis = 0.0f;

        if (scancodes.contains(positive)) {
            axis += scancodes[positive].down ? 1.0f : 0.0f;
        }
        if (scancodes.contains(negative)) {
            axis += scancodes[negative].down ? -1.0f : 0.0f;
        }
        axis = glm::clamp(axis, -1.0f, 1.0f);

        return axis;
    }
}
