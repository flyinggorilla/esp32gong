
idf_component_register(SRCS "Esp32Gong.cpp" "stdlib_noniso.c" "ApiStore.cpp" "CriticalSection.cpp" "DotstarStripe.cpp" 
"DynamicRequestHandler.cpp" "Esp32GongWebServer.cpp" "EspString.cpp" "FileSystem.cpp" "HttpRequestParser.cpp" "HttpResponse.cpp"
"HttpResponseParser.cpp" "I2SPlayer.cpp" "IPAddress.cpp" "MemoryDataStream.cpp" "Ota.cpp" "Storage.cpp" "StorageDataStream.cpp"
"StringParser.cpp" "Url.cpp" "UrlParser.cpp" "Wav.cpp" "WebClient.cpp" "WebServer.cpp" "Wifi.cpp" "Config.cpp" 
    INCLUDE_DIRS "." 
    REQUIRES esp_wifi wpa_supplicant mbedtls esp_https_ota openssl vfs fatfs spiffs app_update driver esp_netif lwip
    PRIV_REQUIRES nvs_flash
    EMBED_TXTFILES "certkey.pem"
    EMBED_FILES "../data/material-design-icons.woff" "../data/material-design-icons.eot" "../data/material-design-icons.ttf" "../data/material-design-icons.svg"
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
