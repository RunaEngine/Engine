set(THIRDPARTY_DIR ${CMAKE_CURRENT_LIST_DIR})

#VCPKG
find_package(zstd CONFIG REQUIRED)
find_package(tomlplusplus CONFIG REQUIRED)

# Dependencies
include(ExternalProject)
include(FetchContent)
include(ExternalProject)
set(FETCHCONTENT_QUIET OFF)

#GRAPHICS LIBRARY
add_subdirectory(${THIRDPARTY_DIR}/sdl)
add_subdirectory(${THIRDPARTY_DIR}/nri)
add_subdirectory(${THIRDPARTY_DIR}/vulkan)
add_subdirectory(${THIRDPARTY_DIR}/slang)
add_subdirectory(${THIRDPARTY_DIR}/glm)

#USER INTERFACE LIBRARY
add_subdirectory(${THIRDPARTY_DIR}/imgui)

#IMAGE LIBRARY
add_subdirectory(${THIRDPARTY_DIR}/stb)

#IMPORT LIBRARY
add_subdirectory(${THIRDPARTY_DIR}/cgltf)

#UTILS LIBRARY
add_subdirectory(${THIRDPARTY_DIR}/bsthreadpool)
add_subdirectory(${THIRDPARTY_DIR}/simdjson)
