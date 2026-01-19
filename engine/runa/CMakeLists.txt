set(RUNA_DIR ${CMAKE_CURRENT_LIST_DIR})

file(GLOB_RECURSE RUNA_SOURCES "${RUNA_DIR}/include/*.h" "${RUNA_DIR}/src/*.cpp")

add_executable(${CMAKE_PROJECT_NAME} ${RUNA_SOURCES})

target_include_directories(${CMAKE_PROJECT_NAME}
        PUBLIC
        ${RUNA_DIR}/include
)

target_link_libraries(${CMAKE_PROJECT_NAME}
        PUBLIC
        runtime
)

include(${CMAKE_SOURCE_DIR}/content/content.cmake)
copy_resources_to_target(${CMAKE_PROJECT_NAME})