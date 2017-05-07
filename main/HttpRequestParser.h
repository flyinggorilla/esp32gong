#ifndef MAIN_HTTPPARSER_H_
#define MAIN_HTTPPARSER_H_

#include "StringParser.h"
#include "UrlParser.h"
#include <string>
#include <list>


#define STATE_Method				0
#define STATE_ParseUrl				1
#define STATE_HttpType				2
#define STATE_SearchEndOfHeaderLine 3
#define STATE_SkipHeader			4
#define STATE_CheckHeaderName		5
#define STATE_CheckHeaderValue		6
#define STATE_ReadContentLength		7
#define STATE_CopyBody				8


class HttpRequestParser {
public:
	HttpRequestParser(int socket);
	virtual ~HttpRequestParser();

	void Init();
	void Clear();

	bool ParseRequest(char* sBuffer, __uint16_t uLen);

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
	int mSocket;
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

#endif /* MAIN_HTTPPARSER_H_ */
