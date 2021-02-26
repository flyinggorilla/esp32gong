idf_component_register(SRCS "Ota.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES esp_system log filesystem web app_update)