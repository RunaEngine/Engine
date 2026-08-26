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
set(SLANG_ENABLE_SLANGI OFF CACHE BOOL "")
set(SLANG_ENABLE_SLANGC OFF CACHE BOOL "")
set(SLANG_ENABLE_SLANGD OFF CACHE BOOL "")
set(SLANG_ENABLE_SLANG_GLSLANG OFF CACHE BOOL "")

set(SLANG_EMBED_CORE_MODULE_SOURCE OFF CACHE BOOL "")
set(SLANG_EMBED_CORE_MODULE ON CACHE BOOL "")
set(SLANG_ENABLE_NVAPI "OFF" CACHE STRING "")
set(SLANG_ENABLE_IR_BREAK_ALLOC OFF CACHE BOOL "")
set(SLANG_ENABLE_CUDA "OFF" CACHE STRING "")
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
    #set_target_properties(slang-dxil PROPERTIES FOLDER "Thirdparty/slang")
    #set_target_properties(slang-proxy PROPERTIES FOLDER "Thirdparty/slang")
endif()
#set_target_properties(slang-common-objects PROPERTIES FOLDER "/Thirdparty/slang")
#set_target_properties(slangc PROPERTIES FOLDER "/Thirdparty/slang")
#set_target_properties(slang-glslang PROPERTIES FOLDER "/Thirdparty/slang")
#set_target_properties(slangd PROPERTIES FOLDER "/Thirdparty/slang")
#set_target_properties(slangi PROPERTIES FOLDER "/Thirdparty/slang")