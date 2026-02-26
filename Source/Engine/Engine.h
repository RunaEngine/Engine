#pragma once

#include "Opengl/Render.h"
#include "Vulkan/Pipeline.h"
#include "Io/Event.h"
#include "Tick.h"
#include "Input.h"
#include "Settings.h"

extern GameUserSettings* GUserSettings;
extern GLRender* GRender;
extern Pipeline* GPipeline;
extern Event* GEvent;
extern Tick* GTick;
extern Input* GInput;