set(CONTENT_DIR ${CMAKE_CURRENT_LIST_DIR})

function(copy_resources_to_target TARGET_NAME)
    # Obtém o diretório onde o executável será gerado
    get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)
    if(TARGET_TYPE STREQUAL "EXECUTABLE")
        set(RESOURCES_DEST_DIR "$<TARGET_FILE_DIR:${TARGET_NAME}>/Resources")

        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CONTENT_DIR}/Resources ${RESOURCES_DEST_DIR}
                COMMENT "Copying resources do exe dir"
        )
    endif()
endfunction()
