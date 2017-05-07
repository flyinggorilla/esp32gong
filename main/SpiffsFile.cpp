/*
 * SpiffsFileHandle.cpp
 * https://github.com/pellepl/spiffs/wiki/Using-spiffs
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
	return  fileHandle >= SPIFFS_OK;
}

const char* SpiffsFile::LastError() {
	switch (SPIFFS_errno(&SpiffsFileSystem::fs)) {
	case SPIFFS_OK : return "OK";
	case SPIFFS_ERR_NOT_MOUNTED : return "not mounted";
	case SPIFFS_ERR_FULL : return "full";
	case SPIFFS_ERR_NOT_FOUND : return "not found";
	case SPIFFS_ERR_END_OF_OBJECT : return "end of object";
	case SPIFFS_ERR_DELETED : return "deleted";
	case SPIFFS_ERR_NOT_FINALIZED        : return "not finalized";
	case SPIFFS_ERR_NOT_INDEX            : return "not index";
	case SPIFFS_ERR_OUT_OF_FILE_DESCS    : return "out of file descriptors";
	case SPIFFS_ERR_FILE_CLOSED          : return "file closed";
	case SPIFFS_ERR_FILE_DELETED         : return "file deleted";
	case SPIFFS_ERR_BAD_DESCRIPTOR       : return "bad descriptor";
	case SPIFFS_ERR_IS_INDEX             : return "is index";
	case SPIFFS_ERR_IS_FREE              : return "is free";
	case SPIFFS_ERR_INDEX_SPAN_MISMATCH  : return "index span mismatch";
	case SPIFFS_ERR_DATA_SPAN_MISMATCH   : return "data span mismatch";
	case SPIFFS_ERR_INDEX_REF_FREE       : return "index ref free";
	case SPIFFS_ERR_INDEX_REF_LU         : return "index ref lu";
	case SPIFFS_ERR_INDEX_REF_INVALID    : return "index ref invalid";
	case SPIFFS_ERR_INDEX_FREE           : return "index free";
	case SPIFFS_ERR_INDEX_LU             : return "index lu";
	case SPIFFS_ERR_INDEX_INVALID        : return "index invalid";
	case SPIFFS_ERR_NOT_WRITABLE         : return "not writable";
	case SPIFFS_ERR_NOT_READABLE         : return "not readable";
	case SPIFFS_ERR_CONFLICTING_NAME     : return "conflicting name";
	case SPIFFS_ERR_NOT_CONFIGURED       : return "not configured";
	case SPIFFS_ERR_NOT_A_FS             : return "not a file system";
	case SPIFFS_ERR_MOUNTED              : return "mounted";
	case SPIFFS_ERR_ERASE_FAIL           : return "erase failed";
	case SPIFFS_ERR_MAGIC_NOT_POSSIBLE   : return "magic not possible";
	case SPIFFS_ERR_NO_DELETED_BLOCKS    : return "no deleted blocks";
	case SPIFFS_ERR_FILE_EXISTS          : return "file exists";
	case SPIFFS_ERR_NOT_A_FILE           : return "not a file";
	case SPIFFS_ERR_RO_NOT_IMPL          : return "RO not implemented";
	case SPIFFS_ERR_RO_ABORTED_OPERATION : return "RO aborted operation";
	case SPIFFS_ERR_PROBE_TOO_FEW_BLOCKS : return "probe too few blocks";
	case SPIFFS_ERR_PROBE_NOT_A_FS       : return "probe not a file system";
	case SPIFFS_ERR_NAME_TOO_LONG        : return "name too long";
	case SPIFFS_ERR_IX_MAP_UNMAPPED      : return "IX map unmapped";
	case SPIFFS_ERR_IX_MAP_MAPPED        : return "IX map mapped";
	case SPIFFS_ERR_IX_MAP_BAD_RANGE     : return "IX map bad range";
	case SPIFFS_ERR_INTERNAL             : return "internal";
	}

	return "unknown";
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

int SpiffsFile::Read(std::string& readbuffer, int maxread) {
	readbuffer.reserve(maxread);
	int read = SPIFFS_read(&SpiffsFileSystem::fs, fileHandle, (void*)readbuffer.data(), maxread);
	readbuffer.resize(read);
	return read;
}

int SpiffsFile::Write(std::string& writebuffer) {
	return SPIFFS_write(&SpiffsFileSystem::fs, fileHandle, (void*)writebuffer.data(), writebuffer.size());
}
