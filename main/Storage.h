/*
 *
 */

#ifndef MAIN_STORAGEHANDLER_H_
#define MAIN_STORAGEHANDLER_H_

#include "DownAndUploadHandler.h"
#include "EspString.h"
#include "FileSystem.h"
//#include "WebClient.h"

#define STORAGE_PROGRESS_NOTYETSTARTED      -1
#define STORAGE_PROGRESS_CONNECTIONERROR    -2
#define STORAGE_PROGRESS_FLASHERROR	        -3
#define STORAGE_PROGRESS_FINISHEDSUCCESS  -200

class Storage : public FileSystem, public DownAndUploadHandler {
public:
	//static void StartUpdateFirmwareTask(const char* url);
	//static int  smErrorCode; //TODO this should provide "feedback" from the static class*/

	/*
	*   get file write progress in 0..100%
	*   @returns in case of an error, it returns negative error codes
	*/
	static int GetProgress();
	static unsigned int GetTimestamp();

public:
	Storage();
	virtual ~Storage();
	bool DownloadFile(String& sUrl);
	

protected:
	bool InternalOnRecvBegin(bool isContentLength, unsigned int contentLength);

public:
	bool OnReceiveBegin(unsigned short int httpStatusCode, bool isContentLength, unsigned int contentLength);
	bool OnReceiveBegin(String& sFilename, unsigned int contentLength);
	bool OnReceiveEnd();
	bool OnReceiveData(char* buf, int len); // override DownloadHandler virtual method



private:
	//WebClient mWebClient;
    unsigned int muActualDataLength = 0;
	unsigned int muContentLength = 0;
	static volatile int miProgress; 
	static volatile unsigned int muTimestamp;
	String mFilename;
};

#endif /* MAIN_STORAGEHANDLER_H_ */
