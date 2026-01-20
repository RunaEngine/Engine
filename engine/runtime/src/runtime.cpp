#include "runtime.h"

namespace runa::runtime
{
    GameUserSettings gameUserSettings = GameUserSettings();
    opengl::Render render = opengl::Render();
    io::Event event = io::Event();
    Tick tick = Tick();
    Input input = Input();
}