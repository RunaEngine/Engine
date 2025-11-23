set(ENGINE_DIR ${CMAKE_CURRENT_LIST_DIR})

include(${ENGINE_DIR}/thirdparty/thirdparty.cmake)

add_subdirectory(${ENGINE_DIR}/config)
add_subdirectory(${ENGINE_DIR}/runtime)
add_subdirectory(${ENGINE_DIR}/runa)

