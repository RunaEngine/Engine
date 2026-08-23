FetchContent_Declare(
        slang
        GIT_REPOSITORY https://github.com/shader-slang/slang.git
        GIT_TAG v2026.16
)

# FIX: Keep essential compiler pipeline features ON to build types correctly
set(SLANG_ENABLE_REPLAYER OFF CACHE BOOL "")
set(SLANG_ENABLE_SPIRV_TOOLS_MIMALLOC OFF CACHE BOOL "")
set(SLANG_ENABLE_SLANG_RHI OFF CACHE BOOL "")
set(SLANG_ENABLE_TESTS OFF CACHE BOOL "")
set(SLANG_OVERRIDE_GLM_PATH OFF CACHE BOOL "")
set(SLANG_ENABLE_SLANGRT OFF CACHE BOOL "")
set(SLANG_ENABLE_SPLIT_DEBUG_INFO OFF CACHE BOOL "")
set(SLANG_STANDARD_MODULE_DEVELOP_BUILD OFF CACHE BOOL "")
set(SLANG_ENABLE_RELEASE_DEBUG_INFO OFF CACHE BOOL "")
set(SLANG_ENABLE_PREBUILT_BINARIES OFF CACHE BOOL "")
set(SLANG_ENABLE_EXAMPLES OFF CACHE BOOL "")
set(SLANG_ENABLE_GFX OFF CACHE BOOL "")

# CRITICAL FIX: Turn these core components ON so header structures resolve perfectly
set(SLANG_ENABLE_SLANGI ON CACHE BOOL "")
set(SLANG_ENABLE_SLANGC ON CACHE BOOL "")
set(SLANG_ENABLE_SLANGD ON CACHE BOOL "")
set(SLANG_ENABLE_SLANG_GLSLANG ON CACHE BOOL "")

set(SLANG_EMBED_CORE_MODULE_SOURCE ON CACHE BOOL "")
set(SLANG_EMBED_CORE_MODULE OFF CACHE BOOL "")
set(SLANG_ENABLE_NVAPI "AUTO" CACHE STRING "")
set(SLANG_ENABLE_IR_BREAK_ALLOC OFF CACHE BOOL "")
set(SLANG_ENABLE_CUDA "AUTO" CACHE STRING "")
set(SLANG_LIB_TYPE "SHARED" CACHE STRING "")

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(SLANG_ENABLE_DXIL ON CACHE BOOL "")
    set(SLANG_ENABLE_DX_ON_VK ON CACHE BOOL "")
endif()


FetchContent_MakeAvailable(slang)

set_target_properties(core PROPERTIES FOLDER "Thirdparty/slang")
set_target_properties(compiler-core PROPERTIES FOLDER "Thirdparty/slang")
set_target_properties(slang PROPERTIES FOLDER "Thirdparty/slang")
set_target_properties(slang-dispatcher PROPERTIES FOLDER "Thirdparty/slang")
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set_target_properties(slang-dxil PROPERTIES FOLDER "Thirdparty/slang")
    set_target_properties(slang-proxy PROPERTIES FOLDER "Thirdparty/slang")
endif()
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_compile_definitions(SLANG_LINUX)
    add_compile_options(-Wno-invalid-constexpr) 
    include_directories(SYSTEM 
        ${slang_SOURCE_DIR}
        ${slang_SOURCE_DIR}/source
        ${slang_SOURCE_DIR}/include
    )
    if(TARGET Runa)
        target_include_directories(Runa PRIVATE ${slang_SOURCE_DIR}/include)
        target_link_libraries(Runa PRIVATE slang)
    endif()
endif()
#set_target_properties(slang-common-objects PROPERTIES FOLDER "/Thirdparty/slang")
#set_target_properties(slangc PROPERTIES FOLDER "/Thirdparty/slang")
#set_target_properties(slang-glslang PROPERTIES FOLDER "/Thirdparty/slang")
#set_target_properties(slangd PROPERTIES FOLDER "/Thirdparty/slang")
#set_target_properties(slangi PROPERTIES FOLDER "/Thirdparty/slang")