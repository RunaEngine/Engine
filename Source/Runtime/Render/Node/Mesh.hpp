#pragma once

#include "Engine/Core/Object.hpp"
#include "Runtime/RHI/wgpu/WGPipeline.hpp"
#include "Runtime/RHI/wgpu/WGVertexBuffer.hpp"
#include "Runtime/RHI/wgpu/WGShader.hpp"

class Mesh : Object
{
private:
    WGVertexBuffer VertexBuffer;
    WGShader Shader;
    SharedPtr<WGPipeline> Pipeline;
public:
    Mesh() = default;
    ~Mesh()
    {

    }

    void Init()
    {

    }
};