set(VULKAN_DIR ${CMAKE_CURRENT_LIST_DIR})

project(vulkan)
file(GLOB_RECURSE VULKAN_SOURCES "${VULKAN_DIR}/include/*.h")
add_library(vulkan INTERFACE ${VULKAN_SOURCES})
target_include_directories(vulkan
        INTERFACE
        "${VULKAN_DIR}/include"
)
set_target_properties(vulkan PROPERTIES FOLDER "/Thirdparty" LINKER_LANGUAGE CXX)
target_compile_definitions(vulkan 
        INTERFACE
        VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL=0
        VK_NO_PROTOTYPES
        VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
)