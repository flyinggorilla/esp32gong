/*
 * Spiffs.h
 *
 *  Created on: 10.04.2017
 *      Author: bernd
 */

#ifndef MAIN_SPIFFSFILESYSTEM_H_
#define MAIN_SPIFFSFILESYSTEM_H_

#define LOG_PAGE_SIZE       256

#include "esp_spiffs.h"
#include "SpiffsFile.h"
#include <stdlib.h>
#include <string>
#include <vector>


/*
 * @brief	Wrapper around the spiffs filesystem for ESP32.
 *
 * It searches for a partition of name "data", type "data", subtype "spiffs".
 * (configure custom partitions in make menuconfig and create a partitions.csv.
 * It might look like this:
 * @code
 * # Name,   Type, SubType, Offset,   Size
 * nvs,      data, nvs,     0x9000,  0x4000
 * otadata,  data, ota,     0xd000,  0x2000
 * phy_init, data, phy,     0xf000,  0x1000
 * factory,  app,  factory, 0x10000,  1M
 * ota_0,    app,  ota_0,   ,         1M
 * ota_1,    app,  ota_1,   ,         1M
 * data, 	  data, spiffs,  ,       896k
 * @endcode
 * Note: the Spiffs class is using static members and is not reentrant.
 *
 */
class SpiffsFileSystem {
friend class SpiffsFile;
public:
	SpiffsFileSystem();
	virtual ~SpiffsFileSystem();
	void test();
	static bool mount();

public:
	static int Errno();


private:
	static spiffs fs; /*! fs.mounted = 0; fs.user_data = NULL; !*/
	static u8_t spiffs_work_buf[LOG_PAGE_SIZE * 2];
	static u8_t spiffs_fds[32 * 4];
	static u8_t spiffs_cache_buf[(LOG_PAGE_SIZE + 32) * 4];


};



#endif /* MAIN_SPIFFSFILESYSTEM_H_ */
