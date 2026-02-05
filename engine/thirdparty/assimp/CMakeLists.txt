FetchContent_Declare(
        ASSIMP
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        TAG v6.0.4
)

if (NOT ASSIMP_POPULATED)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)

    set(ASSIMP_INSTALL_PD OFF CACHE INTERNAL "")
    set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD OFF CACHE INTERNAL "")
    set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
    set(ASSIMP_BUILD_ZLIB OFF CACHE INTERNAL "")
    set(ASSIMP_BUILD_RST_IMPORTER OFF CACHE BOOL "" FORCE)
endif ()
# ASSIMP_BUILD_ZLIB
FetchContent_MakeAvailable(ASSIMP)

set_target_properties(assimp PROPERTIES FOLDER "/engine/thirdparty/assimp")
set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER "/engine/thirdparty/assimp")