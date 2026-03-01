#pragma once

#include "Engine/Core/Object.h"
#include <vector>
#include <glad/glad.h>

class GLElementBuffer : public Object
{
public:
    GLElementBuffer() = default;
    ~GLElementBuffer() override;

    void Init(const std::vector<uint32_t>& indices);
    void Deinit();

    void Bind() const;
    void Unbind() const;

    GLsizeiptr Count() const;

private:
    uint32_t Id = 0;
    GLsizeiptr Size = 0;
};
