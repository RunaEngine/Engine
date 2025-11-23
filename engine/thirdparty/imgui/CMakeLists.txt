FetchContent_Declare(
    ocornutimgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    TAG v1.92.4-docking
)

FetchContent_MakeAvailable(ocornutImgui)

set(IMGUI_SOURCE_DIR ${ocornutimgui_SOURCE_DIR})

project(imgui)

set(IMGUI_SOURCES
        ${IMGUI_SOURCE_DIR}/imconfig.h
        ${IMGUI_SOURCE_DIR}/imgui.h
        ${IMGUI_SOURCE_DIR}/imgui.cpp
        ${IMGUI_SOURCE_DIR}/imgui_internal.h
        ${IMGUI_SOURCE_DIR}/imstb_rectpack.h
        ${IMGUI_SOURCE_DIR}/imstb_textedit.h
        ${IMGUI_SOURCE_DIR}/imstb_truetype.h
        ${IMGUI_SOURCE_DIR}/imgui_demo.cpp
        ${IMGUI_SOURCE_DIR}/imgui_draw.cpp
        ${IMGUI_SOURCE_DIR}/imgui_tables.cpp
        ${IMGUI_SOURCE_DIR}/imgui_widgets.cpp

        ${IMGUI_SOURCE_DIR}/backends/imgui_impl_sdl3.h
        ${IMGUI_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
        ${IMGUI_SOURCE_DIR}/backends/imgui_impl_opengl3.h
        ${IMGUI_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND  IMGUI_SOURCES
        ${IMGUI_SOURCE_DIR}/backends/imgui_impl_win32.h
        ${IMGUI_SOURCE_DIR}/backends/imgui_impl_win32.cpp
        ${IMGUI_SOURCE_DIR}/backends/imgui_impl_vulkan.h
        ${IMGUI_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
    )
    if(MSVC)
        list(APPEND  IMGUI_SOURCES
            ${IMGUI_SOURCE_DIR}/backends/imgui_impl_dx11.h
            ${IMGUI_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
            ${IMGUI_SOURCE_DIR}/backends/imgui_impl_dx12.h
            ${IMGUI_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
        )
    endif()
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND  IMGUI_SOURCES 
        ${IMGUI_SOURCE_DIR}/backends/imgui_impl_vulkan.h
        ${IMGUI_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
    )
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    list(APPEND  IMGUI_SOURCES 
        ${IMGUI_SOURCE_DIR}/backends/imgui_impl_osx.h
        ${IMGUI_SOURCE_DIR}/backends/imgui_impl_osx.mm
        ${IMGUI_SOURCE_DIR}/backends/imgui_impl_metal.h
        ${IMGUI_SOURCE_DIR}/backends/imgui_impl_metal.cpp
    )
endif()

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${IMGUI_SOURCE_DIR} ${ocornutimgui_SOURCE_DIR}/backends)
target_link_libraries(imgui
        PUBLIC
        SDL3::SDL3
        glad
        vulkan
)
set_target_properties(imgui PROPERTIES FOLDER "/engine/thirdparty" LINKER_LANGUAGE CXX)
