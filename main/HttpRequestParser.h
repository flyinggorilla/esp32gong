#ifndef MAIN_HTTPREQUESTPARSER_H_
#define MAIN_HTTPREQUESTPARSER_H_

#include <string>
#include <list>

#include "StringParser.hpp"
#include "UrlParser.h"


class HttpRequestParser {
public:
	HttpRequestParser();
	virtual ~HttpRequestParser();

	void Init();
	void Clear();

	bool ParseRequest(char* sBuffer, __uint16_t uLen);
	bool ParseResponse(char* sBuffer, __uint16_t uLen);

	bool RequestFinished() 	{ return mbFinished; };
	bool IsHttp11() 		{ return mbHttp11; };
	bool IsConnectionClose(){ return mbConClose; };
	bool IsGet()			{ return mbIsGet; };

	std::string& GetUrl() 	{ return mUrl; };
	std::string& GetBody()  { return mBody; };
	std::list<TParam>& GetParams() { return mParams; };

	void SetError(__uint8_t u) { muError = u; };
	__uint8_t GetError()  	{ return muError; };

private:
	//int mSocket;
	UrlParser mUrlParser;
	std::string mUrl;
	std::list<TParam> mParams;
	TParam* mpActParam;

	std::string mBody;
	__uint16_t muContentLength;

	bool mbFinished;
	bool mbHttp11;
	bool mbConClose;
	bool mbIsGet;

	__uint8_t muParseState;
	StringParser mStringParser;

	__uint8_t muCrlfCount;

	__uint8_t muError;
};

#endif /* MAIN_HTTPREQUESTPARSER_H_ */
