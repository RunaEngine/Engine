set(THIRDPARTY_DIR ${CMAKE_CURRENT_LIST_DIR})

#VCPKG

# Dependencies
include(ExternalProject)
include(FetchContent)
include(ExternalProject)
set(FETCHCONTENT_QUIET OFF)

#COMPRESSION LIBRARY
add_subdirectory(${THIRDPARTY_DIR}/zlib)

#IMAGE LIBRARY
#add_subdirectory(${THIRDPARTY_DIR}/libjpeg)
#add_subdirectory(${THIRDPARTY_DIR}/gif)
#add_subdirectory(${THIRDPARTY_DIR}/libpng)
#add_subdirectory(${THIRDPARTY_DIR}/libwebp)

#GRAPHICS LIBRARY
add_subdirectory(${THIRDPARTY_DIR}/vulkan)
add_subdirectory(${THIRDPARTY_DIR}/sdl)
add_subdirectory(${THIRDPARTY_DIR}/glad)
add_subdirectory(${THIRDPARTY_DIR}/glm)

#USER INTERFACE LIBRARY
add_subdirectory(${THIRDPARTY_DIR}/imgui)

#IMAGE LIBRARY
add_subdirectory(${THIRDPARTY_DIR}/sdlimage)
add_subdirectory(${THIRDPARTY_DIR}/stb)

#IMPORT LIBRARY
add_subdirectory(${THIRDPARTY_DIR}/assimp)
