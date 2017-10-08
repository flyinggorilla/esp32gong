/*
 * FileSystem.h
 *
 *  Created on: 06.05.2017
 *      Author: bernd
 */

#ifndef MAIN_FILESYSTEM_H_
#define MAIN_FILESYSTEM_H_

#include <stdlib.h>
#include <stdio.h>
#include <list>
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_system.h"
#include "String.h"

/*
 * @brief
 * FAT filesystem class, to wrap ESP-IDF libraries.
 * uses wear leveling library.
 *
 */

struct TDirEntry {
	String name;
	unsigned long size;
};


class FileSystem {
public:
	FileSystem();
	virtual ~FileSystem();

	bool Mount();
	void Unmount();
	bool ListDirectory(std::list<TDirEntry>& dirList);

	bool Open(String filename, bool write = true);
	bool Write(const char* data, unsigned int size);
	unsigned int Read(char* buf, unsigned int maxlen);
	unsigned int Read(String& readbuffer, unsigned int maxread);
	bool Write(String& writebuffer);
	void Close();
	const char* LastError();
	unsigned int getBytesWritten() { return bytesWritten; }

	bool Delete(String filename);

	unsigned int FreeBytes();
	unsigned int TotalBytes();




private:
	FILE* mpFileHandle = NULL;
	size_t bytesWritten = 0;

};

#endif /* MAIN_FILESYSTEM_H_ */



