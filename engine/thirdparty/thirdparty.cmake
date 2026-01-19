set(THIRDPARTY_DIR ${CMAKE_CURRENT_LIST_DIR})

#VCPKG
find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(libuv CONFIG REQUIRED)

# Dependencies
include(FetchContent)
include(ExternalProject)
set(FETCHCONTENT_QUIET OFF)

#GRAPHICS LIBRARY
add_subdirectory(${THIRDPARTY_DIR}/vulkan)
#add_subdirectory(${THIRDPARTY_DIR}/sdl)
add_subdirectory(${THIRDPARTY_DIR}/glad)
add_subdirectory(${THIRDPARTY_DIR}/glm)

#USER INTERFACE LIBRARY
add_subdirectory(${THIRDPARTY_DIR}/imgui)

#SCRIPT LANGUAGE LIBRARY
add_subdirectory(${THIRDPARTY_DIR}/luau)

#NETWORKING/DATA LIBRARY
add_subdirectory(${THIRDPARTY_DIR}/asio)
add_subdirectory(${THIRDPARTY_DIR}/simdjson)

#IMAGE LIBRARY
add_subdirectory(${THIRDPARTY_DIR}/stb)
