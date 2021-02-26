#ifndef MAIN_HTTPPARSER_H_
#define MAIN_HTTPPARSER_H_

#include "StringParser.h"
#include "UrlParser.h"
#include "EspString.h"
#include <list>


#define STATE_Method					   0
#define STATE_ParseUrl					   1
#define STATE_HttpType					   2
#define STATE_SearchEndOfHeaderLine 	   3
#define STATE_SkipHeader				   4
#define STATE_CheckHeaderName			   5
#define STATE_CheckHeaderValue			   6
#define STATE_ReadContentLength			   7
#define STATE_SearchBoundary			   8
#define STATE_ParseBoundary				   9
#define STATE_ParseFormBody				   10
#define STATE_CopyBody					   11
#define STATE_ProcessMultipartBodyHeaders  12
#define STATE_ProcessMultipartBodyStart	   13
#define STATE_ProcessMultipartBody		   14
#define STATE_ProcessMultipartBodyFilename 15


class DownAndUploadHandler;


class UploadHandler {
public:
	String mUrl;
	DownAndUploadHandler* mpUploadHandler;
};

class HttpRequestParser {
public:
	HttpRequestParser(int socket);
	virtual ~HttpRequestParser();

	void Init(std::list<UploadHandler>* pUploadHandlerList);
	void Clear();

	bool ParseRequest(char* sBuffer, __uint16_t uLen);

	bool RequestFinished() 	{ return mbFinished; };
	bool IsHttp11() 		{ return mbHttp11; };
	bool IsConnectionClose(){ return mbConClose; };
	bool IsGet()			{ return mbIsGet; };

	String& GetUrl() 	{ return mUrl; };
	String& GetBody()  { return mBody; };
	String& GetBoundary() { return mBoundary; }
	std::list<TParam>& GetParams() { return mParams; };

	void SetError(__uint8_t u) { muError = u; mbFinished = true; };
	__uint8_t GetError()  	{ return muError; };

private:
	bool ProcessMultipartBody(char* sBuffer, __uint16_t uLen);

	int mSocket;
	UrlParser mUrlParser;
	String mUrl;
	String mFilename;
	std::list<TParam> mParams;
	TParam* mpActParam;

	std::list<UploadHandler>* mpUploadHandlerList;
	DownAndUploadHandler* mpUploadHandler;

	String mBody;
	String mBoundary;
	__uint32_t muContentLength;
	__uint32_t muActBodyLength;
	
	bool mbParseFormBody;
	bool mbStoreUploadInBody;
	bool mbFinished;
	bool mbHttp11;
	bool mbConClose;
	bool mbIsGet;

	__uint8_t muParseState;
	StringParser mStringParser;

	__uint8_t muCrlfCount;

	__uint8_t muError;
};

#endif /* MAIN_HTTPPARSER_H_ */
