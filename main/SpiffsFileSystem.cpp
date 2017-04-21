/*
 * Spiffs.cpp
 *
 *  Created on: 10.04.2017
 *      Author: bernd
 */

#include "SpiffsFileSystem.h"

#include "sdkconfig.h"
#include <esp_partition.h>
#include <esp_spiffs.h>
#include <esp_log.h>
#include <esp_err.h>

spiffs SpiffsFileSystem::fs; /*! fs.mounted = 0; fs.user_data = NULL; !*/
u8_t SpiffsFileSystem::spiffs_work_buf[LOG_PAGE_SIZE * 2];
u8_t SpiffsFileSystem::spiffs_fds[32 * 4];
u8_t SpiffsFileSystem::spiffs_cache_buf[(LOG_PAGE_SIZE + 32) * 4];

SpiffsFileSystem::SpiffsFileSystem() {
	fs.mounted = 0;
	fs.user_data = NULL;

}

SpiffsFileSystem::~SpiffsFileSystem() {

}

bool SpiffsFileSystem::mount() {

	const esp_partition_t* partition = esp_partition_find_first(
			ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, "data");

	if (partition) {
		ESP_LOGI("SPIFFS", "partition found at address %08x of size %u",
				partition->address, partition->size);
	} else {
		ESP_LOGE("SPIFFS", "SPIFFS partition named 'data' NOT found");
		return false;
	}

	spiffs_config cfg;
	cfg.phys_size = partition->size; // use all spi flash
	cfg.phys_addr = partition->address; // start spiffs at start of spi flash
	cfg.phys_erase_block = 65536; // according to datasheet
	cfg.log_block_size = 65536; // let us not complicate things
	cfg.log_page_size = LOG_PAGE_SIZE; // as we said

	cfg.hal_read_f = (spiffs_read) esp_spi_read;
	cfg.hal_write_f = (spiffs_write) esp_spi_write;
	cfg.hal_erase_f = (spiffs_erase) esp_spi_erase;

	int res = SPIFFS_mount(&fs, &cfg, spiffs_work_buf, spiffs_fds,
			sizeof(spiffs_fds), spiffs_cache_buf, sizeof(spiffs_cache_buf), 0);

	if (res == SPIFFS_ERR_NOT_A_FS) {
		ESP_LOGW("SPIFFS", "No File System found ---> formatting SPIFFS")
		SPIFFS_format(&fs);
		int res = SPIFFS_mount(&fs, &cfg, spiffs_work_buf, spiffs_fds,
				sizeof(spiffs_fds), spiffs_cache_buf, sizeof(spiffs_cache_buf),
				0);
	}

	ESP_LOGI("SPIFFS", "mount res: %i\n", res);
	return true;
}

// ESP32 sizing is 64K for the logical block size and 256 for the logical
void SpiffsFileSystem::test() { //TODO REMOVE TEST METHOD

	mount();

	char buf[12];

	// Surely, I've mounted spiffs before entering here

	spiffs_file fd = SPIFFS_open(&fs, "my_file",
	SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
	if (SPIFFS_write(&fs, fd, (u8_t *) "Hello world", 12) < 0)
		printf("errno %i\n", SPIFFS_errno(&fs));
	SPIFFS_close(&fs, fd);

	fd = SPIFFS_open(&fs, "my_file", SPIFFS_RDWR, 0);
	if (SPIFFS_read(&fs, fd, (u8_t *) buf, 12) < 0)
		printf("errno %i\n", SPIFFS_errno(&fs));
	SPIFFS_close(&fs, fd);

	printf("--> %s <--\n", buf);
}

int SpiffsFileSystem::Errno() { //static!!
	return SPIFFS_errno(&fs);
}

