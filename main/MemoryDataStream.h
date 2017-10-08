#include "Wav.h"

#ifndef MAIN_MEMORYDATASTREAM_H_
#define MAIN_MEMORYDATASTREAM_H_

class MemoryDataStream : public DataStream {
public:
    MemoryDataStream(const void* pData, unsigned int dataLength);
    virtual ~MemoryDataStream();
    
    bool Open();
    bool Read(char* buf, unsigned int len);
    void Close();

private:
    const uint8_t* mpData = NULL;
    unsigned int mPos = 0;
    unsigned int mDataLen;   
};


#endif /*  MAIN_MEMORYDATASTREAM_H_ */


