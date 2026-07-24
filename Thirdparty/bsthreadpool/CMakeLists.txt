set(BSTHREADPOOL_DIR ${CMAKE_CURRENT_LIST_DIR})

project(bsthreadpool)
file(GLOB_RECURSE BSTHREADPOOL_SOURCES "${BSTHREADPOOL_DIR}/include/*.h")
add_library(bsthreadpool INTERFACE ${BSTHREADPOOL_SOURCES})
target_include_directories(bsthreadpool
        INTERFACE
        "${BSTHREADPOOL_DIR}/include"
)
set_target_properties(bsthreadpool PROPERTIES FOLDER "Thirdparty" LINKER_LANGUAGE CXX)