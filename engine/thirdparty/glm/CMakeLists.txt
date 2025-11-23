FetchContent_Declare(
    GLM
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    TAG 1.0.1
)

if (NOT GLM_POPULATED)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(GLM_BUILD_INSTAL OFF CACHE INTERNAL "")
    set(GLM_BUILD_TESTS OFF CACHE INTERNAL "")
    set(GLM_TEST_ENABLE OFF CACHE INTERNAL "")
    set(GLM_ENABLE_SIMD_AVX2 ON CACHE INTERNAL "")
    set(GLM_ENABLE_SIMD_SSE4_2 ON CACHE INTERNAL "")
endif ()


FetchContent_MakeAvailable(GLM)
set_target_properties(glm PROPERTIES FOLDER "/engine/thirdparty")