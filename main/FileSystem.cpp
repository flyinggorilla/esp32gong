/*
 * FileSystem.cpp
 *

 */

#include "FileSystem.h"

#include <stdlib.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_system.h"

static const char *TAG = "FATFS";

// Mount path for the partition
static const char *base_path = "/spiflash";

// Handle of the wear levelling library instance
static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;


FileSystem::FileSystem() {

}

FileSystem::~FileSystem() {
	// TODO Auto-generated destructor stub
}

bool FileSystem::Mount() {
    ESP_LOGI(TAG, "Mounting FAT filesystem");
    // To mount device we need name of device partition, define base_path
    // and allow format partition in case if it is new one and was not formated before
    const esp_vfs_fat_mount_config_t mount_config = { 	4 , // max_files
    													true }; //format_if_mount_failed

    esp_err_t err = esp_vfs_fat_spiflash_mount(base_path, "data", &mount_config, &s_wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (0x%x)", err);
        return false;
    }
    return true;
}

void FileSystem::Unmount() {
    // Unmount FATFS
    ESP_LOGI(TAG, "Unmounting FAT filesystem");
    ESP_ERROR_CHECK( esp_vfs_fat_spiflash_unmount(base_path, s_wl_handle));
}


bool FileSystem::Open(String filename, bool write) {
	ESP_LOGI(TAG, "Opening file");
	String s = base_path + filename;
	mpFileHandle = fopen(s.c_str(), write ? "wb" : "rb");
	if (mpFileHandle == NULL) {
	    ESP_LOGE(TAG, "Failed to open file.");
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

