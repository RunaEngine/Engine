cmake_minimum_required(VERSION 3.28)
project(Runa VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 23)
set(USE_FOLDERS ON)
add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)

include(Contents/Content.cmake)
include(Thirdparty/Thirdparty.cmake)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ENGINE_BUILD_DEBUG true)
    set(ENGINE_BUILD_RELEASE false)
else()
    set(ENGINE_BUILD_DEBUG false)
    set(ENGINE_BUILD_RELEASE true)
endif()

set(ENGINE_NAME ${CMAKE_PROJECT_NAME})
set(ENGINE_MAJOR_VERSION ${CMAKE_PROJECT_VERSION_MAJOR})
set(ENGINE_MINOR_VERSION ${CMAKE_PROJECT_VERSION_MINOR})
set(ENGINE_PATH_VERSION ${CMAKE_PROJECT_VERSION_PATCH})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Source/Config.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/Source/Config.hpp @ONLY)

file(GLOB_RECURSE ENGINE_SOURCES Source/*.hpp Source/*.cpp)
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
add_executable(Runa)
target_include_directories(Runa
        PUBLIC
        Source/
)
target_sources(Runa
        PRIVATE
        Source/main.cpp
)
target_sources(Runa
        PUBLIC
        ${ENGINE_SOURCES}
)
target_link_libraries(Runa
        PUBLIC
        #GRAPHICS LIBRARY
        #vulkan
        #vma
        wgpu_native
        SDL3-static
        sdlwgpu
        #glad
        glm-header-only
        #slang
        #USER INTERFACE LIBRARY
        #imgui
        #IMAGE LIBRARY
        stb
        #SDL3_image-shared
        #OBJ LIBRARY
        cgltf
        #COMPRESSION
        zlibstatic
        #UTILS
        bsthreadpool
)

#set_target_properties(Runa PROPERTIES FOLDER "Source")

function(copy_target_shared_dlls target)
        foreach(lib IN LISTS ARGN)
                if(${lib} STREQUAL "wgpu_native")
                        add_custom_command(TARGET ${target} POST_BUILD
                                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                                        "${WGPU_BINARY}"
                                        $<TARGET_FILE_DIR:${target}>
                        )
                elseif(TARGET ${lib})
                        add_custom_command(TARGET ${target} POST_BUILD
                                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                                        $<TARGET_FILE:${lib}>
                                        $<TARGET_FILE_DIR:${target}>
                        )
                
                else()
                        message(VERBOSE "copy_target_shared_dlls: target '${lib}' não encontrado; pulando cópia")
                endif()
        endforeach()
endfunction()

copy_target_shared_dlls(Runa
        #slang
        miniz
        wgpu_native
)

#include(Content/Content.cmake)
copy_resources_to_target(Runa)


