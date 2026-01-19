set(VULKAN_DIR ${CMAKE_CURRENT_LIST_DIR})

project(vulkan)
file(GLOB_RECURSE VULKAN_SOURCES "${VULKAN_DIR}/include/*.h")
add_library(vulkan INTERFACE ${VULKAN_SOURCES})
target_include_directories(vulkan
        INTERFACE
        "${VULKAN_DIR}/include"
)
set_target_properties(vulkan PROPERTIES FOLDER "/engine/thirdparty" LINKER_LANGUAGE CXX)
