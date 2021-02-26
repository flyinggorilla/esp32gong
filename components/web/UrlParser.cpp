#include "UrlParser.h"
#include "EspString.h"
#include <_ansi.h>
#include <cctype>


//---------------------------------------------------------


UrlParser::UrlParser() {
	Init();
}

UrlParser::~UrlParser() {
}

void UrlParser::Init(){
	muState = STATE_ParseUrl;
	muInDecode = 0;
	muDecodedValue = 0;
}

void UrlParser::ConsumeChar(char c, String& url, TParam* pParam){

	if (!c)
		return;

	switch(muState){
		case STATE_ParseUrl:
			if (c == '?')
				muState = STATE_UrlComplete;
			else
				url += (char)tolower(c);
			break;
		case STATE_UrlComplete:
		case STATE_ParamComplete:
		case STATE_ParseParamName:
			if (c == '&'){
				muState = STATE_ParamComplete;
				muInDecode = 0;
			}
			else if (c == '='){
				muState = STATE_ParseParamValue;
				muInDecode = 0;
			}
			else if (c == '%'){
				muInDecode = 1;
				muDecodedValue = 0;
			}
			else if (pParam){
				if (muInDecode){
					if (ProcessHash(c))
						pParam->paramName += (char)muDecodedValue;
				}
				else
					pParam->paramName += (c == '+') ? ' ' : (char)tolower(c);
				muState = STATE_ParseParamName;
			}
			break;
		case STATE_ParseParamValue:
			if (c == '&'){
				muState = STATE_ParamComplete;
				muInDecode = 0;
			}
			else if (c == '%'){
				muInDecode = 1;
				muDecodedValue = 0;
			}
			else if (pParam){
				if (muInDecode){
					if (ProcessHash(c))
						pParam->paramValue += (char)muDecodedValue;
				}
				else
					pParam->paramValue += (c == '+') ? ' ' : c;
			}
			break;

	}
}

void UrlParser::SignalEnd(){
	switch(muState){
		case STATE_ParseUrl:          
			muState = STATE_UrlComplete;
			break;
		case STATE_ParseParamName:
		case STATE_ParseParamValue:
			muState = STATE_ParamComplete;
			break;
	}
}

bool UrlParser::ProcessHash(char c){

	__uint8_t u;

	if ((c >= '0') && (c <= '9'))
		u = c - '0';
	else if ((c >= 'a') && (c <= 'f'))
		u = c - 'a' + 10;
	else if ((c >= 'A') && (c <= 'F'))
		u = c - 'A' + 10;
	else
		u = 0;

	muDecodedValue *= 16;
	muDecodedValue += u;
	muInDecode++;
	if (muInDecode >= 3){
		muInDecode = 0;
		return true;
	}
	return false;
}



