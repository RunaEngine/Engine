set(RUNTIME_DIR ${CMAKE_CURRENT_LIST_DIR})

project(runtime)
file(GLOB_RECURSE RUNTIME_SOURCES "${RUNTIME_DIR}/include/*.h" "${RUNTIME_DIR}/src/*.cpp")
add_library(runtime STATIC ${RUNTIME_SOURCES})
target_include_directories(runtime
        PUBLIC
        "${RUNTIME_DIR}/include"
)
target_link_libraries(runtime
        PUBLIC
        config
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
set_target_properties(runtime PROPERTIES FOLDER "/engine/runtime" LINKER_LANGUAGE CXX)