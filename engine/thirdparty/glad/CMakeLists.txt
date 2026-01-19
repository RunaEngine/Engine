set(GLAD_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})

project(glad)

file(GLOB_RECURSE GLAD_SOURCES "${GLAD_SOURCE_DIR}/include/*.h" "${GLAD_SOURCE_DIR}/src/*.c")
add_library(glad STATIC ${GLAD_SOURCES})
target_include_directories(glad
        PUBLIC
        "${GLAD_SOURCE_DIR}/include"
)
target_link_libraries(glad
        PUBLIC
        vulkan
)
set_target_properties(glad PROPERTIES FOLDER "/engine/thirdparty")