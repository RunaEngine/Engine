#pragma once

#include <dawn/webgpu_cpp.h>

namespace WGPUtils
{
    wgpu::TextureFormat RemoveSrgbSuffix(wgpu::TextureFormat format)
    {
        switch (format)
        {
        case wgpu::TextureFormat::RGBA8UnormSrgb:
            return wgpu::TextureFormat::RGBA8Unorm;
        case wgpu::TextureFormat::BGRA8UnormSrgb:
            return wgpu::TextureFormat::BGRA8Unorm;
        case wgpu::TextureFormat::BC1RGBAUnormSrgb:
            return wgpu::TextureFormat::BC1RGBAUnorm;
        case wgpu::TextureFormat::BC2RGBAUnormSrgb:
            return wgpu::TextureFormat::BC2RGBAUnorm;
        case wgpu::TextureFormat::BC3RGBAUnormSrgb:
            return wgpu::TextureFormat::BC3RGBAUnorm;
        case wgpu::TextureFormat::BC7RGBAUnormSrgb:
            return wgpu::TextureFormat::BC7RGBAUnorm;
        case wgpu::TextureFormat::ETC2RGB8UnormSrgb:
            return wgpu::TextureFormat::ETC2RGB8Unorm;
        case wgpu::TextureFormat::ETC2RGB8A1UnormSrgb:
            return wgpu::TextureFormat::ETC2RGB8A1Unorm;
        case wgpu::TextureFormat::ETC2RGBA8UnormSrgb:
            return wgpu::TextureFormat::ETC2RGBA8Unorm;
        case wgpu::TextureFormat::ASTC4x4UnormSrgb:
            return wgpu::TextureFormat::ASTC4x4Unorm;
        default:
            return format;
        }
    }
}