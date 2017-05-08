#ifndef MAIN_DOWNLOADHANDLER_H_
#define MAIN_DOWNLOADHANDLER_H_


class DownloadHandler {
public:
	virtual bool OnReceiveBegin(unsigned short int httpStatusCode, bool isContentLength, unsigned int contentLength) =0;
	virtual void OnReceiveEnd() =0;
	virtual bool OnReceiveData(char* buf, int len) =0; // =0 means pure virtual; must override
};


#endif // MAIN_DOWNLOADHANDLER_H_
