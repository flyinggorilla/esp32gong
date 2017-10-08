#include "StorageDataStream.h"
#include "Storage.h"

extern Storage storage;

StorageDataStream::StorageDataStream(String filename) {
    mFilename = filename;
}

StorageDataStream::~StorageDataStream() {
    storage.Close();
}

bool StorageDataStream::Open() {
    return storage.Open(mFilename, false);
}

bool StorageDataStream::Read(char* buf, unsigned int len) {
    return storage.Read(buf, len) == len;
}

void StorageDataStream::Close() {
    return storage.Close();
}
