set(VMA_DIR ${CMAKE_CURRENT_LIST_DIR})

project(vma)
file(GLOB_RECURSE VMA_SOURCES "${VMA_DIR}/include/*.h")
add_library(vma INTERFACE ${VMA_SOURCES})
target_include_directories(vma
        INTERFACE
        "${VMA_DIR}/include"
)
set_target_properties(vma PROPERTIES FOLDER "Thirdparty" LINKER_LANGUAGE CXX)