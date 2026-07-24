set(THIRDPARTY_DIR ${CMAKE_CURRENT_LIST_DIR})

#VCPKG

# Dependencies
include(ExternalProject)
include(FetchContent)
include(ExternalProject)
set(FETCHCONTENT_QUIET OFF)

#COMPRESSION LIBRARY
add_subdirectory(${THIRDPARTY_DIR}/zlib)

#GRAPHICS LIBRARY
add_subdirectory(${THIRDPARTY_DIR}/wgpu)
#add_subdirectory(${THIRDPARTY_DIR}/vulkan)
#add_subdirectory(${THIRDPARTY_DIR}/vma)
add_subdirectory(${THIRDPARTY_DIR}/sdl)
add_subdirectory(${THIRDPARTY_DIR}/sdlwgpu)
#add_subdirectory(${THIRDPARTY_DIR}/glad)
add_subdirectory(${THIRDPARTY_DIR}/glm)

#SHADER LIBRARY
#add_subdirectory(${THIRDPARTY_DIR}/slang)

#USER INTERFACE LIBRARY
#add_subdirectory(${THIRDPARTY_DIR}/imgui)

#IMAGE LIBRARY
#add_subdirectory(${THIRDPARTY_DIR}/sdlimage)
add_subdirectory(${THIRDPARTY_DIR}/stb)

#IMPORT LIBRARY
#add_subdirectory(${THIRDPARTY_DIR}/assimp)
add_subdirectory(${THIRDPARTY_DIR}/cgltf)

#UTILS LIBRARY
add_subdirectory(${THIRDPARTY_DIR}/bsthreadpool)
