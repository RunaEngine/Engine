FetchContent_Declare(
        ZLIB
        URL https://www.zlib.net/current/zlib.tar.gz
)

if (NOT ZLIB_POPULATED)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)

    # Configurações básicas de build
    set(ZLIB_BUILD_EXAMPLES OFF CACHE INTERNAL "")
endif ()

FetchContent_MakeAvailable(ZLIB)

# Definir variáveis esperadas pelo find_package
if (NOT ZLIB_LIBRARY)
    if (MSVC)
        set(ZLIB_LIBRARY zlibstatic CACHE FILEPATH "")
    else ()
        set(ZLIB_LIBRARY z CACHE FILEPATH "")
    endif ()
endif ()

if (NOT ZLIB_INCLUDE_DIR)
    set(ZLIB_INCLUDE_DIR ${zlib_SOURCE_DIR} CACHE PATH "")
endif ()

set(ZLIB_FOUND TRUE)
set(ZLIB_LIBRARIES ${ZLIB_LIBRARY})
set(ZLIB_INCLUDE_DIRS ${ZLIB_INCLUDE_DIR})