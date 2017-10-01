#ifndef MAIN_DOWNANDUPLOADHANDLER_H_
#define MAIN_DOWNANDUPLOADHANDLER_H_

class String;

class DownAndUploadHandler {
public:
	virtual bool OnReceiveBegin(unsigned short int httpStatusCode, bool isContentLength, unsigned int contentLength) =0;
	virtual bool OnReceiveBegin(String& sUrl, unsigned int contentLength) =0;
	virtual bool OnReceiveEnd() =0;
	virtual bool OnReceiveData(char* buf, int len) =0; // =0 means pure virtual; must override
};


#endif // MAIN_DOWNLOADHANDLER_H_
