#https://github.com/gfx-rs/wgpu-native/releases/download/v29.0.1.1/wgpu-windows-x86_64-msvc-release.zip

string(TOLOWER "${CMAKE_SYSTEM_NAME}" TARGET_OS)
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" TARGET_ARCH)

set(WGPU_OS "")
set(WGPU_ARCH "")
set(WGPU_SUFFIX "release.zip")

if (TARGET_OS STREQUAL "windows")
        set(WGPU_OS "windows")
        if (TARGET_ARCH MATCHES "arm64" OR TARGET_ARCH MATCHES "aarch64")
                set(WGPU_ARCH "aarch64-msvc")
        else()
                set(WGPU_ARCH "x86_64-msvc")
        endif()

elseif (TARGET_OS STREQUAL "darwin" OR TARGET_OS STREQUAL "macos")
        set(WGPU_OS "macos")
        set(WGPU_ARCH "aarch64")

elseif (TARGET_OS STREQUAL "ios")
        set(WGPU_OS "ios")
        set(WGPU_ARCH "aarch64")

elseif (TARGET_OS STREQUAL "linux")
        set(WGPU_OS "linux")
        if (TARGET_ARCH MATCHES "arm64" OR TARGET_ARCH MATCHES "aarch64")
                set(WGPU_ARCH "aarch64")
        else()
                set(WGPU_ARCH "x86_64")
        endif()

elseif (TARGET_OS STREQUAL "android")
        set(WGPU_OS "android")
        if (ANDROID_ABI STREQUAL "arm64-v8a")
                set(WGPU_ARCH "aarch64")
        elseif (ANDROID_ABI STREQUAL "x86_64")
                set(WGPU_ARCH "x86_64")
        else()
                message(FATAL_ERROR "Android arch not supported: ${ANDROID_ABI}")
        endif()

else()
        message(FATAL_ERROR "Target OS not found: ${CMAKE_SYSTEM_NAME}")
endif()

set(WGPU_ASSET_NAME "wgpu-${WGPU_OS}-${WGPU_ARCH}-${WGPU_SUFFIX}")
set(WGPU_URL "https://github.com/gfx-rs/wgpu-native/releases/download/v29.0.1.1/${WGPU_ASSET_NAME}")

FetchContent_Declare(
        libwgpu
        URL ${WGPU_URL}
)

#FetchContent_GetProperties(libwgpu)
FetchContent_MakeAvailable(libwgpu)

set(WGPU_INCLUDE_DIR "${libwgpu_SOURCE_DIR}/include")
set(WGPU_LIB_DIR "${libwgpu_SOURCE_DIR}/lib")

add_library(wgpu_native INTERFACE)

target_include_directories(wgpu_native INTERFACE "${WGPU_INCLUDE_DIR}")

if (TARGET_OS STREQUAL "windows")
        find_library(WGPU_LIB NAMES wgpu_native wgpu_native.dll PATHS "${WGPU_LIB_DIR}" NO_DEFAULT_PATH)
        set(WGPU_BINARY "${WGPU_LIB_DIR}/wgpu_native.dll" CACHE STRING "Path to WGPU binary library")
elseif (TARGET_OS STREQUAL "darwin" OR TARGET_OS STREQUAL "macos" OR TARGET_OS STREQUAL "ios")
        find_library(WGPU_LIB NAMES wgpu_native libwgpu_native PATHS "${WGPU_LIB_DIR}" NO_DEFAULT_PATH)
        set(WGPU_BINARY "${WGPU_LIB_DIR}/libwgpu_native.so" CACHE STRING "Path to WGPU binary library")
else()
        find_library(WGPU_LIB NAMES wgpu_native libwgpu_native PATHS "${WGPU_LIB_DIR}" NO_DEFAULT_PATH)
        set(WGPU_BINARY "${WGPU_LIB_DIR}/libwgpu_native.so" CACHE STRING "Path to WGPU binary library")
endif()

if(NOT WGPU_LIB)
        message(FATAL_ERROR "[WGPU] Lib file not found: ${WGPU_LIB}")
endif()

target_link_libraries(wgpu_native INTERFACE "${WGPU_LIB}")