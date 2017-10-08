#include "MemoryDataStream.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

MemoryDataStream::MemoryDataStream(const void* pData, unsigned int dataLength) {
    mpData = (uint8_t*)pData;
    mDataLen = dataLength;
}

MemoryDataStream::~MemoryDataStream() {
    
}

bool MemoryDataStream::Open() {
    mPos = 0;
    return true;
}

bool MemoryDataStream::Read(char* buf, unsigned int len) {
    if (mPos + len > mDataLen) {
        memset(buf, 0, len);
        return false;
    }
    memcpy(buf, (const void*)(&(mpData[mPos])), len);
    mPos += len;
    return true;
}

void MemoryDataStream::Close() {

}
