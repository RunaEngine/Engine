FetchContent_Declare(
    GLM
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    TAG 1.0.3
)

if (NOT GLM_POPULATED)
    set(GLM_BUILD_INSTAL OFF CACHE INTERNAL "")
    set(GLM_BUILD_TESTS OFF CACHE INTERNAL "")
    set(GLM_TEST_ENABLE OFF CACHE INTERNAL "")
    set(GLM_ENABLE_SIMD_AVX2 ON CACHE INTERNAL "" FORCE)
    set(GLM_ENABLE_SIMD_SSE4_2 ON CACHE INTERNAL "" FORCE)
endif ()


FetchContent_MakeAvailable(GLM)
set_target_properties(glm PROPERTIES FOLDER "/Thirdparty")