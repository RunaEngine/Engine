FetchContent_Declare(
    cgltf
    GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
    TAG v1.15
)

FetchContent_MakeAvailable(cgltf)
set(CGLTF_SOURCE_DIR ${cgltf_SOURCE_DIR})

project(gltf)
set(CGLTF_SOURCES
        ${CGLTF_SOURCE_DIR}/cgltf.h
        ${CGLTF_SOURCE_DIR}/cgltf_write.h
    )
add_library(gltf INTERFACE ${CGLTF_SOURCES})
target_include_directories(gltf
        INTERFACE
        "${CGLTF_DIR}"
)

set_target_properties(gltf PROPERTIES FOLDER "/engine/thirdparty" LINKER_LANGUAGE C)