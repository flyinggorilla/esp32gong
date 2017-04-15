
#ifndef __ESP_SPIFFS_H__
#define __ESP_SPIFFS_H__

#include "spiffs.h"

#ifdef __cplusplus
extern "C" {
#endif


s32_t esp32_spi_flash_read(u32_t addr, u32_t size, u8_t *dst);
s32_t esp32_spi_flash_write(u32_t addr, u32_t size, const u8_t *src);
s32_t esp32_spi_flash_erase(u32_t addr, u32_t size);

#define esp_spi_read  (spiffs_read *)esp32_spi_flash_read
#define esp_spi_write (spiffs_write *)esp32_spi_flash_write
#define esp_spi_erase (spiffs_erase *)esp32_spi_flash_erase

#ifdef __cplusplus
}
#endif

#endif  // __ESP_SPIFFS_H__
