set(RUNTIME_DIR ${CMAKE_CURRENT_LIST_DIR})

project(runtime)
file(GLOB_RECURSE RUNTIME_SOURCES "${RUNTIME_DIR}/include/*.h" "${RUNTIME_DIR}/src/*.cpp")
add_library(runtime STATIC ${RUNTIME_SOURCES})
target_include_directories(runtime
        PUBLIC
        "${RUNTIME_DIR}/include"
)
target_link_libraries(runtime
        PUBLIC
        config
        #EVENT LIBRARY
        $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>
        #GRAPHICS LIBRARY
        SDL3::SDL3
        glad
        glm
        #USER INTERFACE LIBRARY
        imgui
        #SCRIPT LANGUAGE LIBRARY
        Luau.VM
        Luau.Compiler
        #CRYPTO LIBRARY
        OpenSSL::SSL
        OpenSSL::Crypto
        #NETWORKING/DATA LIBRARY
        asio
        simdjson
        #IMAGE LIBRARY
        $<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image-shared>,SDL3_image::SDL3_image-shared,SDL3_image::SDL3_image-static>
        stb
        #COMPRESSION
        ZLIB::ZLIB
)
set_target_properties(runtime PROPERTIES FOLDER "/engine/runtime" LINKER_LANGUAGE CXX)