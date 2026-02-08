#pragma once

#include "Engine/Core/Object.h"
#include <cstdint>

class Tick : public Object
{
public:
    Tick() = default;

    void UpdateCurrentTick();
    void UpdateDeltaTime();

    uint64_t ElapsedNS();
    uint64_t DeltaNS();
    double Delta();

private:
    uint64_t CurrentTickNS = 0;
    uint64_t DeltaTimeNS = 0;
};
