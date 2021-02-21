idf_component_register(SRCS "Esp32Gong.cpp" "stdlib_noniso.c" "ApiStore.cpp" "CriticalSection.cpp" "DotstarStripe.cpp" 
"DynamicRequestHandler.cpp" "Esp32GongWebServer.cpp" "EspString.cpp" "FileSystem.cpp" "HttpRequestParser.cpp" "HttpResponse.cpp"
"HttpResponseParser.cpp" "I2SPlayer.cpp" "IPAddress.cpp" "MemoryDataStream.cpp" "Ota.cpp" "Storage.cpp" "StorageDataStream.cpp"
"StringParser.cpp" "Url.cpp" "UrlParser.cpp" "Wav.cpp" "WebClient.cpp" "WebServer.cpp" "Wifi.cpp" "Config.cpp" 
    INCLUDE_DIRS "." 
    REQUIRES esp_wifi wpa_supplicant mbedtls esp_https_ota openssl vfs fatfs spiffs app_update driver esp_netif lwip
    PRIV_REQUIRES nvs_flash)

