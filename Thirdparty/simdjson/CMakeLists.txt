set(SIMDJSON_DIR ${CMAKE_CURRENT_LIST_DIR})

project(simdjson)
file(GLOB_RECURSE SIMDJSON_SOURCES "${SIMDJSON_DIR}/include/*.h" "${SIMDJSON_DIR}/src/*.cpp")
add_library(simdjson STATIC ${SIMDJSON_SOURCES})
target_include_directories(simdjson
        PUBLIC
        "${SIMDJSON_DIR}/include"
)
set_target_properties(simdjson PROPERTIES FOLDER "/engine/thirdparty")