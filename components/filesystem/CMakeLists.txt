idf_component_register(SRCS "FileSystem.cpp" 
                       INCLUDE_DIRS "."
                       REQUIRES esp_system spiffs espstring vfs fatfs)