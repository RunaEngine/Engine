#pragma once

#include "Settings.hpp"
#include "Vulkan/Pipeline.hpp"
#include "Io/Event.hpp"
#include "Tick.hpp"
#include "Input.hpp"

extern GameUserSettings* GUserSettings = new GameUserSettings();
extern Pipeline* GPipeline = new Pipeline();
extern Event* GEvent = new Event();
extern Tick* GTick = new Tick();
extern Input* GInput = new Input();