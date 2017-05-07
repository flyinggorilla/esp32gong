#include "HttpRequestParser.h"
#include "freertos/FreeRTOS.h"


HttpRequestParser::HttpRequestParser(int socket) {
	mSocket = socket;

	Init();
}

HttpRequestParser::~HttpRequestParser() {
}

void HttpRequestParser::Init(){
	Clear();

	muError = 0;
	mbFinished = false;
	mbConClose = true;
	mUrlParser.Init();
	mpActParam = NULL;
	muContentLength = 0;
	muParseState = STATE_Method;
	mStringParser.Init();
	mStringParser.AddStringToParse("get");
	mStringParser.AddStringToParse("post");
}

void HttpRequestParser::Clear(){
	mUrl.clear();
	mParams.clear();
	mBody.clear();
}

bool HttpRequestParser::ParseRequest(char* sBuffer, __uint16_t uLen){

	__uint16_t uPos = 0;
	while (uPos < uLen){
		char c = sBuffer[uPos];
		uPos++;

		switch (muParseState){
			case STATE_Method:
				if (c == ' '){
					__uint8_t uFound;
					if (!mStringParser.Found(uFound))
						return SetError(1), false;
					mbIsGet = uFound ? false : true;
					muParseState = STATE_ParseUrl;
				}
				else{
					if (!mStringParser.ConsumeChar(c))
						return SetError(2), false;
				}
				break;

			case STATE_ParseUrl:
				if (c == ' '){
					muParseState = STATE_HttpType;
					mUrlParser.SignalEnd();
					mStringParser.Init();
					mStringParser.AddStringToParse("http/1.0");
					mStringParser.AddStringToParse("http/1.1");
				}
				else
					mUrlParser.ConsumeChar(c, mUrl, mpActParam);
				switch (mUrlParser.GetState()){
					case STATE_UrlComplete:
					case STATE_ParamComplete:
						if (c != ' '){
							mParams.emplace_back();
							mpActParam = &mParams.back();
						}
						break;
				}
				break;

			case STATE_HttpType:
				if ((c == 10) || (c == 13)){
					__uint8_t uFound;
					if (!mStringParser.Found(uFound))
						return SetError(3), false;
					mbHttp11 = uFound ? true : false;
					mbConClose = !mbHttp11;
					muCrlfCount = 1;
					muParseState = STATE_SearchEndOfHeaderLine;
				}
				else{
					if (!mStringParser.ConsumeChar(c))
						return SetError(4), false;
				}
				break;

			case STATE_SearchEndOfHeaderLine:
				if ((c != 10) && (c != 13)){
					muParseState = STATE_CheckHeaderName;
					mStringParser.Init();
					mStringParser.AddStringToParse("connection");
					if (mbIsGet)
						mStringParser.AddStringToParse("content-length");
				}
				else{
					if (++muCrlfCount == 4){
						if (mbIsGet){
							mbFinished = true;
							return true;
						}
						else
							muParseState = STATE_CopyBody;
					}
				}
				break;

			case STATE_SkipHeader:
				if ((c == 10) || (c == 13)){
					muCrlfCount = 1;
					muParseState = STATE_SearchEndOfHeaderLine;
				}
				break;

			case STATE_CheckHeaderName:
				if (c == ':'){
					__uint8_t uFound;
					if (mStringParser.Found(uFound)){
						if (!uFound){
							muParseState = STATE_CheckHeaderValue;
							mStringParser.Init();
							mStringParser.AddStringToParse("close");
							mStringParser.AddStringToParse("keep-alive");
						}
						else{
							muParseState = STATE_ReadContentLength;
							muContentLength = 0;
						}
					}
					else
						muParseState = STATE_SkipHeader;
				}
				else{
					if (!mStringParser.ConsumeChar(c)){
						if ((c == 10) || (c == 13))
							muParseState = STATE_SearchEndOfHeaderLine;
						else
							muParseState = STATE_SkipHeader;
					}
				}
				break;

			case STATE_CheckHeaderValue:
				if ((c == 10) || (c == 13)){
					__uint8_t u;
					if (mStringParser.Found(u)){
						mbConClose = u ? false : true;
					}
					muCrlfCount = 1;
					muParseState = STATE_SearchEndOfHeaderLine;
				}
				else{
					if (!mStringParser.ConsumeChar(c))
						muParseState = STATE_SkipHeader;
				}
				break;

			case STATE_ReadContentLength:
				if ((c == 10) || (c == 13)){
					muCrlfCount = 1;
					muParseState = STATE_SearchEndOfHeaderLine;
				}
				else{
					if ((c >= '0') && (c<= '9')){
						muContentLength *= 10;
						muContentLength += c - '0';
					}
				}
				break;

			case STATE_CopyBody:
				if (mbIsGet)
					return SetError(7), false;
				if (uPos < uLen){
					mBody.append(sBuffer[uPos], uLen - uPos);
					mbFinished = mBody.size() >= muContentLength;
					return true;
				}
				break;

		}
	}
	return true;
}
