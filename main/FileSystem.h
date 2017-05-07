/*
 * FileSystem.h
 *
 *  Created on: 06.05.2017
 *      Author: bernd
 */

#ifndef MAIN_FILESYSTEM_H_
#define MAIN_FILESYSTEM_H_

#include <string>
#include <stdlib.h>

#include <stdio.h>
#include <string.h>
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_system.h"

/*
 * @brief
 * FAT filesystem class, to wrap ESP-IDF libraries.
 * uses wear leveling library.
 *
 */

class FileSystem {
public:
	FileSystem();
	virtual ~FileSystem();

	bool Mount();
	void Unmount();

	bool Open(std::string s, bool write = true);
	bool Write(const char* data, unsigned int size);
	unsigned int Read(char* buf, unsigned int maxlen);
	unsigned int Read(std::string& readbuffer, unsigned int maxread);
	bool Write(std::string& writebuffer);
	void Close();
	const char* LastError();
	unsigned int getBytesWritten() { return bytesWritten; }


private:
	FILE* mpFileHandle = NULL;
	size_t bytesWritten = 0;

};

#endif /* MAIN_FILESYSTEM_H_ */



