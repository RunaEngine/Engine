#pragma once

#include <map>
#include <SDL3/SDL.h>
#include <glm/common.hpp>
#include <glm/vec2.hpp>

namespace runa::runtime {
    class input_c {
    public:
        input_c();
        ~input_c();

        void update_event(SDL_Event &event);

        bool is_mouse_button_pressed(int mouseflag);
        bool is_key_pressed(SDL_Scancode scancode);

        glm::vec2 get_input_vector(SDL_Scancode positive_y, SDL_Scancode negative_y, SDL_Scancode positive_x, SDL_Scancode negative_x);
        float get_input_axis(SDL_Scancode positive, SDL_Scancode negative);
    private:
        std::unordered_map<SDL_Scancode, SDL_KeyboardEvent> scancodes;
        std::unordered_map<int, SDL_MouseButtonEvent> mouseflags;
    };

    extern input_c Input;
}
