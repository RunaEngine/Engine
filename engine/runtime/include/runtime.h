#pragma once

#include "opengl/render.h"
#include "io/event.h"
#include "tick.h"
#include "input.h"
#include "settings.h"

namespace runa::runtime
{
    extern GameUserSettings gameUserSettings;
    extern opengl::Render render;
    extern io::Event event;
    extern Tick tick;
    extern Input input;
}
