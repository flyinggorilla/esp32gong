/*
 * FileSystem.cpp
 *

 */

#include "FileSystem.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_system.h"

static const char *TAG = "FileSystem";

// Mount path for the partition
static const char* BASE_PATH = "/spiffs";
static const char* PARTITION_LABEL = "data";

FileSystem::FileSystem() {

}

FileSystem::~FileSystem() {
	// TODO Auto-generated destructor stub
}

bool FileSystem::Mount() {
	ESP_LOGI(TAG, "Mounting SPIFFS filesystem");
	
    esp_vfs_spiffs_conf_t conf = {
      .base_path = BASE_PATH,
      .partition_label = PARTITION_LABEL,
      .max_files = 5,
      .format_if_mount_failed = true
    };
    
    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%d)", ret);
        }
        return false;
    }
    
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(PARTITION_LABEL, &total, &used);
    if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to get SPIFFS partition information");
		return false;
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    return true;
}

void FileSystem::Unmount() {

	// Unmount FATFS
    ESP_LOGI(TAG, "Unmounting SPIFFS filesystem");
    ESP_ERROR_CHECK( esp_vfs_spiffs_unregister(PARTITION_LABEL));
}


bool FileSystem::Open(String filename, bool write) {
	ESP_LOGI(TAG, "Opening file");
	String s = BASE_PATH + String("/") + filename;
	mpFileHandle = fopen(s.c_str(), write ? "wb" : "rb");
	if (mpFileHandle == NULL) {
	    ESP_LOGE(TAG, "Failed to open file %s. %i %s (%s)", s.c_str(), errno, strerror(errno), write ? "wb" : "rb");
	    return false;
	}
	return true;
}

bool FileSystem::Write(const char* data, unsigned int size) {
	if (!mpFileHandle) return false;
	return fwrite(data, 1, size, mpFileHandle) == size;
}

bool FileSystem::Write(String& writebuffer) {
	if (!mpFileHandle) return 0;
	return Write(writebuffer.c_str(), writebuffer.length());
}


unsigned int FileSystem::Read(char* buf, unsigned int len) {
	if (!mpFileHandle) return 0;
	return fread(buf, len, 1, mpFileHandle);
}

unsigned int FileSystem::Read(String& readbuffer, unsigned int maxread) {
	readbuffer.resize(maxread);
	size_t read = Read((char*)readbuffer.c_str(), maxread);
	if (read != maxread) {
		readbuffer.resize(read);
	}
	return read;
}

void FileSystem::Close() {
    fclose(mpFileHandle);
    mpFileHandle = NULL;
}

unsigned int FileSystem::FreeBytes() {
	size_t totalBytes;
	size_t usedBytes;
	ESP_ERROR_CHECK(esp_spiffs_info(PARTITION_LABEL, &totalBytes, &usedBytes));
	return totalBytes - usedBytes;
}


unsigned int FileSystem::TotalBytes() {
	size_t totalBytes;
	size_t usedBytes;
	ESP_ERROR_CHECK(esp_spiffs_info(PARTITION_LABEL, &totalBytes, &usedBytes));
	return totalBytes;
}




