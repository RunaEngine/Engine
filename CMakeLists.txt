cmake_minimum_required(VERSION 3.25)
project(Runa VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 23)
set(USE_FOLDERS ON)
add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)

include(Content/Content.cmake)
include(Thirdparty/Thirdparty.cmake)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ENGINE_BUILD_DEBUG ON)
else()
    set(ENGINE_BUILD_RELEASE ON)
endif()

set(ENGINE_NAME ${CMAKE_PROJECT_NAME})
set(ENGINE_VERSION ${CMAKE_PROJECT_VERSION})
set(ENGINE_MAJOR_VERSION ${CMAKE_PROJECT_VERSION_MAJOR})
set(ENGINE_MINOR_VERSION ${CMAKE_PROJECT_VERSION_MINOR})
set(ENGINE_PATH_VERSION ${CMAKE_PROJECT_VERSION_PATCH})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Source/Config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/Source/Config.h @ONLY)

file(GLOB_RECURSE ENGINE_SOURCES Source/*.h Source/*.cpp)
#[[
list(REMOVE_ITEM ENGINE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Source/Main.cpp")

add_library(Runtime STATIC ${ENGINE_SOURCES})
target_include_directories(Runtime
        PUBLIC
        Source
)
target_link_libraries(Runtime
        PUBLIC
        #GRAPHICS LIBRARY
        vulkan
        SDL3-static
        glad
        glm
        #USER INTERFACE LIBRARY
        imgui
        #IMAGE LIBRARY
        SDL3_image-static
        #OBJ LIBRARY
        assimp
)
set_target_properties(Runtime PROPERTIES FOLDER "/Source" LINKER_LANGUAGE CXX)

list(APPEND ENGINE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Source/Main.cpp")
]]
add_executable(Runa ${ENGINE_SOURCES})
target_include_directories(Runa
        PUBLIC
        Source
)
target_link_libraries(Runa
        PUBLIC
        #GRAPHICS LIBRARY
        vulkan
        SDL3-static
        glad
        glm
        #USER INTERFACE LIBRARY
        imgui
        #IMAGE LIBRARY
        SDL3_image-static
        #OBJ LIBRARY
        assimp
        #COMPRESSION
        zlibstatic
)
set_target_properties(Runa PROPERTIES FOLDER "/Source" LINKER_LANGUAGE CXX)

include(Content/Content.cmake)
copy_resources_to_target(Runa)


