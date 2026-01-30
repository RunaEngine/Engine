FetchContent_Declare(
    FASTGLTF
    URL https://github.com/spnda/fastgltf/archive/refs/tags/v0.9.0.tar.gz
)

if (NOT FASTGLTF_POPULATED)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(FASTGLTF_COMPILE_AS_CPP20 ON CACHE INTERNAL "")
    set(FASTGLTF_DISABLE_CUSTOM_MEMORY_POOL ON CACHE INTERNAL "")
endif ()

FetchContent_MakeAvailable(FASTGLTF)
set_target_properties(fastgltf PROPERTIES FOLDER "/engine/thirdparty" CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)