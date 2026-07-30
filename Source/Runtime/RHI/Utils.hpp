#pragma once

#if defined(SDL_PLATFORM_MACOS) || defined(SDL_PLATFORM_IOS)
#  include <QuartzCore/CAMetalLayer.h>
#  if defined(SDL_PLATFORM_MACOS)
#    include <Cocoa/Cocoa.h>
#  else
#    include <UIKit/UIKit.h>
#  endif
#elif defined(SDL_PLATFORM_WIN32)
#  include <windows.h>
#endif
#include <dawn/webgpu_cpp.h>

namespace WGPUtils {
    wgpu::TextureFormat RemoveSrgbSuffix(wgpu::TextureFormat format) {
        switch (format) {
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

    wgpu::Surface GetWGPUSurfaceFromSDL3(const wgpu::Instance &instance, SDL_Window *window) {
        SDL_PropertiesID props = SDL_GetWindowProperties(window);

#if defined(SDL_PLATFORM_MACOS)
        {
            NSWindow *nsWindow = (__bridge
            NSWindow *)SDL_GetPointerProperty(
                props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
            if (!nsWindow) {
                return nullptr;
            }

            [nsWindow.contentView setWantsLayer:YES];
            CAMetalLayer *metalLayer = [CAMetalLayer layer];
            [nsWindow.contentView setLayer:metalLayer];

            wgpu::SurfaceSourceMetalLayer metalSource = {};
            metalSource.layer = (__bridge
            void *)metalLayer;

            wgpu::SurfaceDescriptor surfaceDesc = {};
            surfaceDesc.nextInChain = &metalSource;

            return instance.CreateSurface(&surfaceDesc);
        }

#elif defined(SDL_PLATFORM_IOS)
        {
            UIWindow *uiWindow = (__bridge
            UIWindow *)SDL_GetPointerProperty(
                props, SDL_PROP_WINDOW_UIKIT_WINDOW_POINTER, nullptr);
            if (!uiWindow) {
                return nullptr;
            }

            UIView *uiView = uiWindow.rootViewController.view;
            CAMetalLayer *metalLayer = [CAMetalLayer new];
            metalLayer.opaque = true;
            metalLayer.frame = uiView.frame;
            metalLayer.drawableSize = uiView.frame.size;
            [uiView.layer addSublayer:metalLayer];

            wgpu::SurfaceSourceMetalLayer metalSource = {};
            metalSource.layer = (__bridge
            void *)metalLayer;

            wgpu::SurfaceDescriptor surfaceDesc = {};
            surfaceDesc.nextInChain = &metalSource;

            return instance.CreateSurface(&surfaceDesc);
        }

#elif defined(SDL_PLATFORM_WIN32)
        {
            HWND hwnd = (HWND) SDL_GetPointerProperty(
                props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
            if (!hwnd) {
                return nullptr;
            }

            wgpu::SurfaceSourceWindowsHWND hwndSource = {};
            hwndSource.hinstance = GetModuleHandle(nullptr);
            hwndSource.hwnd = hwnd;

            wgpu::SurfaceDescriptor surfaceDesc = {};
            surfaceDesc.nextInChain = &hwndSource;

            return instance.CreateSurface(&surfaceDesc);
        }

#elif defined(SDL_PLATFORM_ANDROID)
        {
            void *nativeWindow = SDL_GetPointerProperty(
                props, SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER, nullptr);
            if (!nativeWindow) {
                return nullptr;
            }

            wgpu::SurfaceSourceAndroidNativeWindow androidSource = {};
            androidSource.window = nativeWindow;

            wgpu::SurfaceDescriptor surfaceDesc = {};
            surfaceDesc.nextInChain = &androidSource;

            return instance.CreateSurface(&surfaceDesc);
        }

#elif defined(SDL_PLATFORM_LINUX)
        {
            const char *videoDriver = SDL_GetCurrentVideoDriver();

            if (videoDriver && SDL_strcmp(videoDriver, "wayland") == 0) {
                struct wl_display *waylandDisplay = (struct wl_display *) SDL_GetPointerProperty(
                    props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
                struct wl_surface *waylandSurface = (struct wl_surface *) SDL_GetPointerProperty(
                    props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);

                if (!waylandDisplay || !waylandSurface) {
                    return nullptr;
                }

                wgpu::SurfaceSourceWaylandSurface waylandSource = {};
                waylandSource.sType = wgpu::SType::SurfaceSourceWaylandSurface;
                waylandSource.display = waylandDisplay;
                waylandSource.surface = waylandSurface;

                wgpu::SurfaceDescriptor surfaceDesc = {};
                surfaceDesc.nextInChain = &waylandSource;

                return instance.CreateSurface(&surfaceDesc);
            }

            return nullptr;
        }
#elif defined(SDL_PLATFORM_ANDROID)
        {
            void *nativeWindow = SDL_GetPointerProperty(
                props, SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER, nullptr);
            if (!nativeWindow) {
                return nullptr;
            }

            wgpu::SurfaceSourceAndroidNativeWindow androidSource = {};
            androidSource.window = nativeWindow;

            wgpu::SurfaceDescriptor surfaceDesc = {};
            surfaceDesc.nextInChain = &androidSource;

            return instance.CreateSurface(&surfaceDesc);
        }
#elif defined(__EMSCRIPTEN__)
        {
            wgpu::SurfaceSourceCanvasHTMLSelector_Emscripten canvasSource = {};
            canvasSource.selector = "canvas";

            wgpu::SurfaceDescriptor surfaceDesc = {};
            surfaceDesc.nextInChain = &canvasSource;

            return instance.CreateSurface(&surfaceDesc);
        }

#else
#  error "Plataforma nao suportada em GetWGPUSurfaceFromSDL3"
#endif
    }
}
