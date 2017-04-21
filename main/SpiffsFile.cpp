/*
 * SpiffsFileHandle.cpp
 *
 *  Created on: 11.04.2017
 *      Author: bernd
 */

#include "SpiffsFile.h"

#include <esp_partition.h>
#include <esp_spiffs.h>
#include <esp_log.h>
#include <esp_err.h>

#include "SpiffsFileSystem.h"


SpiffsFile::SpiffsFile() {

}


SpiffsFile::~SpiffsFile() {
	Close();
}

bool SpiffsFile::Open(std::string s) {
	fileHandle = SPIFFS_open(&SpiffsFileSystem::fs, s.c_str(), SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
	return fileHandle != NULL;
}

int SpiffsFile::Write(char* data, int size) {
	int written = SPIFFS_write(&SpiffsFileSystem::fs, fileHandle, data, size);
	if (written > 0) {
		bytesWritten += written;
	}
	return written;
}

int SpiffsFile::Read(char* buf, int maxlen) {
	return SPIFFS_read(&SpiffsFileSystem::fs, fileHandle, buf, maxlen);
}

void SpiffsFile::Close() {
	SPIFFS_close(&SpiffsFileSystem::fs, fileHandle);
}

std::vector<char> SpiffsFile::Read(int maxread) {
	std::vector<char> buf;
	buf.reserve(maxread);
	buf.resize(SPIFFS_read(&SpiffsFileSystem::fs, fileHandle, (void*)buf.data(), maxread));
	return buf;
}

int SpiffsFile::Write(std::vector<char> vector) {
	return SPIFFS_write(&SpiffsFileSystem::fs, fileHandle, (void*)vector.data(), vector.size());
}
