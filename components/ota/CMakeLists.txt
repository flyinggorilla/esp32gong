idf_component_register(SRCS "Ota.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES esp_system filesystem web app_update)