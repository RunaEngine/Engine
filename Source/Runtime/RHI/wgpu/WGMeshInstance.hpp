#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/RHI/wgpu/WGInstanceBuffer.hpp"
#include "Runtime/RHI/wgpu/WGBuffer.hpp"
#include <glm/gtc/type_ptr.hpp>

class WGMeshInstance : Object
{
private:
    std::vector<WGInstance> Instances;
    WGBuffer Buffer;

public:
    WGMeshInstance();
    ~WGMeshInstance();
};
