
idf_component_register(SRCS "Esp32Gong.cpp" "Storage.cpp" 
"DynamicRequestHandler.cpp" "Esp32GongWebServer.cpp" "Wifi.cpp" "Config.cpp" 
"MemoryDataStream" "StorageDataStream"
    INCLUDE_DIRS "." 
    REQUIRES filesystem ota web i2splayer espstring esp_wifi app_update driver esp_netif lwip
    dotstarstripe nvs_flash 
    EMBED_FILES "gong.wav" "material-design-icons.woff" "material-design-icons.eot" "material-design-icons.ttf" "material-design-icons.svg"
    EMBED_FILES ${PROJECT_DIR}/main/index.html.gzip
    )

add_custom_target(gzipindexhtml DEPENDS ${PROJECT_DIR}/main/index.html.gzip)
add_custom_command(OUTPUT ${PROJECT_DIR}/main/index.html.gzip
    COMMAND python ${PROJECT_DIR}/togzip.py ${PROJECT_DIR}/main/index.html ${PROJECT_DIR}/main/index.html.gzip
    DEPENDS ${PROJECT_DIR}/main/index.html
    VERBATIM
)


#message("$COMPONENT_DIR==${COMPONENT_DIR}")
#message("$PROJECT_NAME==${PROJECT_NAME}")
#message("$PROJECT_DIR==${PROJECT_DIR}")
add_dependencies(${COMPONENT_LIB} gzipindexhtml)
set_property(TARGET gzipindexhtml APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${PROJECT_DIR}/main/index.html.gzip)
