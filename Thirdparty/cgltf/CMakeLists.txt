FetchContent_Declare(
        cgltf
        GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
)

FetchContent_MakeAvailable(cgltf)

set(CGLTF_SOURCE_DIR ${cgltf_SOURCE_DIR})

project(cgltf)
set(CGLTF_SOURCES
        ${CGLTF_SOURCE_DIR}/cgltf.h
        ${CGLTF_SOURCE_DIR}/cgltf_write.h
)
add_library(cgltf INTERFACE ${CGLTF_SOURCES})
target_include_directories(cgltf
        INTERFACE
        ${CGLTF_SOURCE_DIR}
)
target_compile_definitions(cgltf
        INTERFACE
        CGLTF_IMPLEMENTATION
)

set_target_properties(cgltf PROPERTIES FOLDER "Thirdparty" LINKER_LANGUAGE C)