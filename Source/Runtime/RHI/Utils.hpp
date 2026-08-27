#pragma once


namespace NRIUtils
{
    uint64_t GetAlignedSize(size_t size, size_t alignment) 
    {
        return (size + alignment - 1) & ~(alignment - 1);
    }
}
