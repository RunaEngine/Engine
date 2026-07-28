set(SDLWGPU_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})

project(sdlwgpu)

file(GLOB_RECURSE SDLWGPU_SOURCES "${SDLWGPU_SOURCE_DIR}/include/*.h" "${SDLWGPU_SOURCE_DIR}/src/*.c")
add_library(sdlwgpu STATIC ${SDLWGPU_SOURCES})
target_include_directories(sdlwgpu
        PUBLIC
        "${SDLWGPU_SOURCE_DIR}/include"
)
target_link_libraries(sdlwgpu
        PUBLIC
        SDL3::SDL3-shared
        dawn::webgpu_dawn
)
set_target_properties(sdlwgpu PROPERTIES FOLDER "Thirdparty")