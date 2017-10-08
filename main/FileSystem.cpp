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
static const char *BASE_PATH = "/spiffs";
static const char *PARTITION_LABEL = "data";

FileSystem::FileSystem()
{
}

FileSystem::~FileSystem()
{
	// TODO Auto-generated destructor stub
}

bool FileSystem::Mount()
{
	ESP_LOGI(TAG, "Mounting SPIFFS filesystem");

	esp_vfs_spiffs_conf_t conf = {
		.base_path = BASE_PATH,
		.partition_label = PARTITION_LABEL,
		.max_files = 5,
		.format_if_mount_failed = true};

	// Use settings defined above to initialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is an all-in-one convenience function.
	esp_err_t ret = esp_vfs_spiffs_register(&conf);

	if (ret != ESP_OK)
	{
		if (ret == ESP_FAIL)
		{
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		}
		else if (ret == ESP_ERR_NOT_FOUND)
		{
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		}
		else
		{
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%d)", ret);
		}
		return false;
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(PARTITION_LABEL, &total, &used);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to get SPIFFS partition information");
		return false;
	}
	else
	{
		ESP_LOGI(TAG, "SPIFFS file-system: total: %d, used: %d", total, used);
	}

	return true;
}

bool FileSystem::ListDirectory(std::list<TDirEntry> &dirList)
{

	DIR *dir;
	struct dirent *entry;

	ESP_LOGD(TAG, "Directory Listing");

	if (!(dir = opendir(BASE_PATH)))
	{
		ESP_LOGE(TAG, "Failed to open directory %s. %s", BASE_PATH, strerror(errno));
		return false;
	}

	dirList.clear();

	while ((entry = readdir(dir)) != NULL)
	{
		if (entry->d_type == DT_DIR)
		{
			//if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;
		}
		else
		{
			struct stat stbuf;
			String f;
			f.printf("%s/%s", BASE_PATH, entry->d_name);

			if (stat(f.c_str(), &stbuf) == -1)
			{
				ESP_LOGE(TAG, "stat error - cannot access file %s. %i %s", f.c_str(), errno, strerror(errno));
			}

			dirList.emplace_back();
			dirList.back().name = entry->d_name;
			dirList.back().size = stbuf.st_size;
			ESP_LOGD(TAG, "  File: %s size: %li", entry->d_name, stbuf.st_size);
		}
	}
	closedir(dir);
	return true;
}

void FileSystem::Unmount()
{

	// Unmount FATFS
	ESP_LOGI(TAG, "Unmounting SPIFFS filesystem");
	ESP_ERROR_CHECK(esp_vfs_spiffs_unregister(PARTITION_LABEL));
}

bool FileSystem::Open(String filename, bool write)
{
	ESP_LOGI(TAG, "Opening file for %s", write ? "WRITING" : "READING");
	String s = BASE_PATH + String("/") + filename;
	mpFileHandle = fopen(s.c_str(), write ? "wb" : "rb");
	if (mpFileHandle == NULL)
	{
		ESP_LOGE(TAG, "Failed to open file %s. %i %s (%s)", s.c_str(), errno, strerror(errno), write ? "wb" : "rb");
		return false;
	}
	return true;
}

bool FileSystem::Write(const char *data, unsigned int size)
{
	if (!mpFileHandle)
		return false;
	return fwrite(data, 1, size, mpFileHandle) == size;
}

bool FileSystem::Write(String &writebuffer)
{
	if (!mpFileHandle)
		return 0;
	return Write(writebuffer.c_str(), writebuffer.length());
}

unsigned int FileSystem::Read(char *buf, unsigned int len)
{
	if (!mpFileHandle)
		return 0;
	return fread(buf, 1, len, mpFileHandle);
}

unsigned int FileSystem::Read(String &readbuffer, unsigned int maxread)
{
	readbuffer.resize(maxread);
	size_t read = Read((char *)readbuffer.c_str(), maxread);
	if (read != maxread)
	{
		readbuffer.resize(read);
	}
	return read;
}

bool FileSystem::Delete(String filename)
{
	String s = BASE_PATH + String("/") + filename;
	if (remove(s.c_str()))
	{
		ESP_LOGE(TAG, "Failed to delete file %s. %s", s.c_str(), strerror(errno));
		return false;
	}
	return true;
}

void FileSystem::Close()
{
	fclose(mpFileHandle);
	mpFileHandle = NULL;
}

unsigned int FileSystem::FreeBytes()
{
	size_t totalBytes;
	size_t usedBytes;
	ESP_ERROR_CHECK(esp_spiffs_info(PARTITION_LABEL, &totalBytes, &usedBytes));
	return totalBytes - usedBytes;
}

unsigned int FileSystem::TotalBytes()
{
	size_t totalBytes;
	size_t usedBytes;
	ESP_ERROR_CHECK(esp_spiffs_info(PARTITION_LABEL, &totalBytes, &usedBytes));
	return totalBytes;
}
