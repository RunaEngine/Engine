FetchContent_Declare(
        libsdl3
        URL https://github.com/libsdl-org/SDL/archive/refs/tags/release-3.4.2.tar.gz
)

set(SDL_TEST_LIBRARY OFF CACHE INTERNAL "")
set(SDL_SHARED ON CACHE INTERNAL "")
set(SDL_STATIC OFF CACHE INTERNAL "")
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
endif ()

FetchContent_MakeAvailable(libsdl3)

set_target_properties(SDL3-shared PROPERTIES FOLDER "/Thirdparty/sdl")
#set_target_properties(SDL_uclibc PROPERTIES FOLDER "/Thirdparty/sdl")