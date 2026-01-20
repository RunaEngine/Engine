set(CGLTF_DIR ${CMAKE_CURRENT_LIST_DIR})

project(cgltf)
file(GLOB_RECURSE CGLTF_SOURCES "${CGLTF_DIR}/include/*.h")
add_library(cgltf INTERFACE ${CGLTF_SOURCES})
target_include_directories(cgltf
        INTERFACE
        "${CGLTF_DIR}/include"
)
target_compile_definitions(cgltf
        INTERFACE
        CGLTF_IMPLEMENTATION
        CGLTF_WRITE_IMPLEMENTATION
)

set_target_properties(cgltf PROPERTIES FOLDER "/engine/thirdparty" LINKER_LANGUAGE C)