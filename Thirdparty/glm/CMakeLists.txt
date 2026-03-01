FetchContent_Declare(
        glmlib
        URL https://github.com/g-truc/glm/archive/refs/tags/1.0.3.tar.gz
)

set(GLM_BUILD_INSTAL OFF CACHE INTERNAL "")
set(GLM_BUILD_TESTS OFF CACHE INTERNAL "")
set(GLM_TEST_ENABLE OFF CACHE INTERNAL "")
set(GLM_ENABLE_SIMD_AVX2 ON CACHE INTERNAL "" FORCE)
set(GLM_ENABLE_SIMD_SSE4_2 ON CACHE INTERNAL "" FORCE)

FetchContent_MakeAvailable(glmlib)

set_target_properties(glm PROPERTIES FOLDER "Thirdparty")