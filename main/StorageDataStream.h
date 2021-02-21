#include "Wav.h"
#include "EspString.h"

#ifndef MAIN_STORAGEDATASTREAM_H_
#define MAIN_STORAGEDATASTREAM_H_

class StorageDataStream : public DataStream {
public:
    StorageDataStream(String filename);
    virtual ~StorageDataStream();

    bool Open();
    bool Read(char* buf, unsigned int len);
    void Close();

private:
    String mFilename;
};


#endif /* MAIN_STORAGEDATASTREAM_H_ */


