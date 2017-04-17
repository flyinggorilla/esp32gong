#ifndef MAIN_URL_PARSER_H_
#define MAIN_URL_PARSER_H_

#include "freertos/FreeRTOS.h"
#include <string>


#define MAX_Params		10

#define STATE_ParseUrl			1
#define STATE_UrlComplete		2
#define STATE_ParseParamName	3
#define STATE_ParseParamValue	4
#define STATE_ParamComplete		5


struct TParam{
	std::string paramName;
	std::string paramValue;
};

class UrlParser {
public:
	UrlParser();
	virtual ~UrlParser();

	void Init();

	void ConsumeChar(char c, std::string& url, TParam* pParam);
	void SignalEnd();

	__uint8_t GetState() { return muState; };

private:
	bool ProcessHash(char c);

private:
	__uint8_t muState;

	__uint8_t muInDecode;
	__uint8_t muDecodedValue;

};

#endif
