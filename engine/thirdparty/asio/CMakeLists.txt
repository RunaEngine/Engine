set(ASIO_DIR ${CMAKE_CURRENT_LIST_DIR})

project(asio)
file(GLOB_RECURSE ASIO_SOURCES "${ASIO_DIR}/include/*.hpp")
add_library(asio INTERFACE ${ASIO_SOURCES})
target_include_directories(asio
        INTERFACE
        "${ASIO_DIR}/include"
)
#[[target_link_libraries(asio
        INTERFACE
        $<TARGET_NAME_IF_EXISTS:OpenSSL::SSL>
        $<TARGET_NAME_IF_EXISTS:OpenSSL::Crypto>
)]]
set_target_properties(asio PROPERTIES FOLDER "/engine/thirdparty" LINKER_LANGUAGE CXX)