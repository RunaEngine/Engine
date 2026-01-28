FetchContent_Declare(
    asionnonboost
    URL https://sourceforge.net/projects/asio/files/asio/1.36.0%20%28Stable%29/boost_asio_1_36_0.tar.gz/download
)

FetchContent_MakeAvailable(asionnonboost)
set(ASIO_SOURCE_DIR ${asionnonboost_SOURCE_DIR})

project(asio)
file(GLOB_RECURSE ASIO_SOURCES "${ASIO_SOURCE_DIR}/asio/include/*.hpp")
add_library(asio INTERFACE ${ASIO_SOURCES})
target_include_directories(asio
        INTERFACE
        "${ASIO_SOURCE_DIR}/asio/include"
)
#[[target_link_libraries(asio
        INTERFACE
        $<TARGET_NAME_IF_EXISTS:OpenSSL::SSL>
        $<TARGET_NAME_IF_EXISTS:OpenSSL::Crypto>
)]]
set_target_properties(asio PROPERTIES FOLDER "/engine/thirdparty" LINKER_LANGUAGE CXX)