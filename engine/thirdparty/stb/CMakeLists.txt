FetchContent_Declare(
    nothingsstb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    TAG master
)

FetchContent_MakeAvailable(nothingsstb)
set(STB_SOURCE_DIR ${nothingsstb_SOURCE_DIR})

project(stb)
set(STB_SOURCES
        ${STB_SOURCE_DIR}/stb_c_lexer.h
        ${STB_SOURCE_DIR}/stb_connected_components.h
        ${STB_SOURCE_DIR}/stb_divide.h
        ${STB_SOURCE_DIR}/stb_ds.h
        ${STB_SOURCE_DIR}/stb_dxt.h
        ${STB_SOURCE_DIR}/stb_easy_font.h
        ${STB_SOURCE_DIR}/stb_herringbone_wang_tile.h
        ${STB_SOURCE_DIR}/stb_hexwave.h
        ${STB_SOURCE_DIR}/stb_image.h
        ${STB_SOURCE_DIR}/stb_image_resize2.h
        ${STB_SOURCE_DIR}/stb_image_write.h
        ${STB_SOURCE_DIR}/stb_include.h
        ${STB_SOURCE_DIR}/stb_leakcheck.h
        ${STB_SOURCE_DIR}/stb_perlin.h
        ${STB_SOURCE_DIR}/stb_rect_pack.h
        ${STB_SOURCE_DIR}/stb_sprintf.h
        ${STB_SOURCE_DIR}/stb_textedit.h
        ${STB_SOURCE_DIR}/stb_tilemap_editor.h
        ${STB_SOURCE_DIR}/stb_truetype.h
        ${STB_SOURCE_DIR}/stb_vorbis.c
        ${STB_SOURCE_DIR}/stb_voxel_render.h
    )
add_library(stb STATIC ${STB_SOURCES})
target_include_directories(stb
        PUBLIC
        ${STB_SOURCE_DIR}
)
target_compile_definitions(stb
        PUBLIC
        STB_IMAGE_IMPLEMENTATION
        STB_IMAGE_WRITE_IMPLEMENTATION
        STB_IMAGE_RESIZE_IMPLEMENTATION
)

set_target_properties(stb PROPERTIES FOLDER "/engine/thirdparty" LINKER_LANGUAGE C)