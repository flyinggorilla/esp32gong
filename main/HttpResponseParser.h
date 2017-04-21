#ifndef MAIN_HTTPREQUESTPARSER_HPP_
#define MAIN_HTTPREQUESTPARSER_HPP_

#include <string>
#include <list>

#include "DownloadHandler.h"
#include "StringParser.hpp"




class HttpResponseParser {
public:
	HttpResponseParser();
	virtual ~HttpResponseParser();

	void Init(DownloadHandler* pOptionalDownloadHandler = NULL);
	void Clear();

	bool ParseResponse(char* sBuffer, unsigned int uLen);
	bool ResponseFinished() { return mbFinished; };
	bool IsHttp11() 		{ return mbHttp11; };
	bool IsConnectionClose(){ return mbConClose; };
	std::string& GetBody()  { return mBody; };
	unsigned int GetContentLength() { return muActualContentLength; }
	void SetError(short u) { muError = u; };
	short GetError()  	{ return muError; };

private:
	std::string mBody;
	unsigned int muContentLength;
	unsigned int muActualContentLength;

	bool mbFinished;
	bool mbHttp11;
	bool mbConClose;
	bool mbContentLength;
	unsigned short muStatusCode;
	std::string msContentType;
	DownloadHandler* mpDownloadHandler;
	uint8_t muParseState;
	StringParser mStringParser;
	uint8_t muCrlfCount;
	uint8_t muError;
};



#endif /* MAIN_HTTPREQUESTPARSER_HPP_ */
