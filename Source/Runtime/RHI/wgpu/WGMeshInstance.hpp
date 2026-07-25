#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/RHI/wgpu/WGInstanceBuffer.hpp"
#include "Runtime/RHI/wgpu/WGBuffer.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <webgpu/wgpu.h>
#include <array>

class WGMeshInstance : Object
{
private:
    std::vector<WGInstance> Instances;
    WGBuffer Buffer;
public:
    WGMeshInstance();
    ~WGMeshInstance();
};
