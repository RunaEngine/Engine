FetchContent_Declare(
    SDL3
    URL https://github.com/libsdl-org/SDL/releases/download/release-3.2.26/SDL3-3.2.26.tar.gz
)

if (NOT SDL3_POPULATED)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(SDL_TEST_LIBRARY OFF CACHE INTERNAL "")
    set(SDL_SHARED OFF CACHE INTERNAL "")
    set(SDL_STATIC ON CACHE INTERNAL "")
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        set(SDL_X11 OFF CACHE INTERNAL "")
        set(SDL_JACK_SHARED OFF CACHE INTERNAL "")
        set(SDL_SNDIO_SHARED OFF CACHE INTERNAL "")
        set(SDL_KMSDRM_SHARED OFF CACHE INTERNAL "")
        set(SDL_HIDAPI_LIBUSB_SHARED OFF CACHE INTERNAL "")
        set(SDL_WAYLAND_LIBDECOR_SHARED OFF CACHE INTERNAL "")
        set(SDL_ALSA_SHARED OFF CACHE INTERNAL "")
        set(SDL_PIPEWIRE_SHARED OFF CACHE INTERNAL "")
        set(SDL_PULSEAUDIO_SHARED: OFF CACHE INTERNAL "")
        set(SDL_WAYLAND_SHARED OFF CACHE INTERNAL "")
    endif()
endif ()

FetchContent_MakeAvailable(SDL3)
set_target_properties(SDL3-static PROPERTIES FOLDER "/engine/thirdparty/sdl")
set_target_properties(SDL_uclibc PROPERTIES FOLDER "/engine/thirdparty/sdl")