
idf_component_register(SRCS "Esp32Gong.cpp" "ApiStore.cpp" "CriticalSection.cpp" "DotstarStripe.cpp" 
"DynamicRequestHandler.cpp" "Esp32GongWebServer.cpp" "FileSystem.cpp"
"I2SPlayer.cpp" "IPAddress.cpp" "MemoryDataStream.cpp" "Ota.cpp" "Storage.cpp" "StorageDataStream.cpp"
"Wav.cpp" "Wifi.cpp" "Config.cpp" 
    INCLUDE_DIRS "." 
    REQUIRES espweb espstring esp_wifi esp_https_ota vfs fatfs spiffs app_update driver esp_netif lwip
    PRIV_REQUIRES nvs_flash
    EMBED_FILES "../data/gong.wav" "../data/material-design-icons.woff" "../data/material-design-icons.eot" "../data/material-design-icons.ttf" "../data/material-design-icons.svg"
    )

add_custom_command(OUTPUT indexhtml.h
    COMMAND python ${PROJECT_DIR}/data2h.py ${PROJECT_DIR}/data/index.html ${PROJECT_DIR}/main/indexhtml.h
    DEPENDS ${PROJECT_DIR}/data/index.html
    VERBATIM
)


#message("$COMPONENT_DIR==${COMPONENT_DIR}")
#message("$PROJECT_NAME==${PROJECT_NAME}")
#message("$PROJECT_DIR==${PROJECT_DIR}")
add_custom_target(gzipindexhtml DEPENDS indexhtml.h)
add_dependencies(${COMPONENT_LIB} gzipindexhtml)

set_property(DIRECTORY "${COMPONENT_DIR}" APPEND PROPERTY
            ADDITIONAL_MAKE_CLEAN_FILES indexhtml.h)

set_property(DIRECTORY "${PROJECT_DIR}" APPEND PROPERTY
            ADDITIONAL_MAKE_CLEAN_FILES ${PROJECT_DIR}/main/indexhtml.h)

#target_add_binary_data(${PROJECT_NAME}.elf main/indexhtml.h BINARY DEPENDS testtarget)
