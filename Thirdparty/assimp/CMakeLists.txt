FetchContent_Declare(
        assimplib
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v6.0.5
)

set(ASSIMP_INSTALL_PD OFF CACHE INTERNAL "")
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD OFF CACHE INTERNAL "")
set(BUILD_SHARED_LIBS ON CACHE INTERNAL "")
set(ASSIMP_BUILD_ZLIB OFF CACHE INTERNAL "")
set(ASSIMP_BUILD_RST_IMPORTER OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_DOUBLE_PRECISION ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(assimplib)

set_target_properties(assimp PROPERTIES FOLDER "Thirdparty/assimp")
target_compile_options(assimp PRIVATE -Wno-error=unused-but-set-variable)
set_target_properties(unit PROPERTIES FOLDER "Thirdparty/assimp")
target_compile_options(unit PRIVATE -Wno-error=unused-but-set-variable)
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER "Thirdparty/assimp")
endif ()