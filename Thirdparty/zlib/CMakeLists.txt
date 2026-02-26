FetchContent_Declare(
        zlibrary
        URL https://www.zlib.net/current/zlib.tar.gz
)

set(ZLIB_BUILD_EXAMPLES OFF CACHE INTERNAL "")

FetchContent_MakeAvailable(zlibrary)

if (NOT ZLIB_LIBRARY)
    set(ZLIB_LIBRARY "zlib" "zlibstatic" CACHE STRING "")
endif ()

set(ZLIB_ROOT ${zlibrary_SOURCE_DIR} CACHE STRING "" FORCE)
set(ZLIB_USE_STATIC_LIBS ON)
find_package(ZLIB REQUIRED)

set_target_properties(zlib PROPERTIES FOLDER "/Thirdparty/zlib")
set_target_properties(zlibstatic PROPERTIES FOLDER "/Thirdparty/zlib")