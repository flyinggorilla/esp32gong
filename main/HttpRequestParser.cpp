#include "HttpRequestParser.h"
#include "freertos/FreeRTOS.h"
#include <esp_log.h>


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
	muActBodyLength = 0;
	muParseState = STATE_Method;
	mStringParser.Init();
	mStringParser.AddStringToParse("get");
	mStringParser.AddStringToParse("post");
}

void HttpRequestParser::Clear(){
	mUrl.clear();
	mParams.clear();
	mBody.clear();
	mBoundary.clear();
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
					if (!mbIsGet){
						mStringParser.AddStringToParse("content-length");
						mStringParser.AddStringToParse("content-type");
					}
					uPos--;
				}
				else{
					if (++muCrlfCount == 4){
						if (mbIsGet){
							mbFinished = true;
							return true;
						}
						else{
							if (mBoundary.size()){
							    muParseState = STATE_ProcessMultipartBodyStart;
								mStringParser.Init();
								mStringParser.AddStringToParse("\r\n\r\n");
							}
							else
								muParseState = STATE_CopyBody;
						}
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
						switch (uFound){
							case 0:
								muParseState = STATE_CheckHeaderValue;
								mStringParser.Init();
								mStringParser.AddStringToParse("close");
								mStringParser.AddStringToParse("keep-alive");
								break;
							case 1:
								muParseState = STATE_ReadContentLength;
								muContentLength = 0;
								break;
							case 2:
								muParseState = STATE_CheckHeaderValue;
								mStringParser.Init();
								mStringParser.AddStringToParse("multipart/form-data");
								break;
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
				if ((c == 10) || (c == 13) || (c==';')){
					__uint8_t u;
					if (mbIsGet){
						if (mStringParser.Found(u)){
							mbConClose = u ? false : true;
						}
						muCrlfCount = 1;
						muParseState = STATE_SearchEndOfHeaderLine;
					}
					else{
						if (mStringParser.Found(u)){
							muParseState = STATE_SearchBoundary;
							mStringParser.Init();
							mStringParser.AddStringToParse("boundary=");
						}
					}
				}
				else{
					if (!mStringParser.ConsumeChar(c, true))
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
			case STATE_SearchBoundary:
				if ((c == 10) || (c == 13)){
					muCrlfCount = 1;
					muParseState = STATE_SearchEndOfHeaderLine;
				}
				else{
					__uint8_t u;
					mStringParser.ConsumeCharSimple(c);
					if (mStringParser.Found(u)){
						mBoundary.clear();
						muParseState = STATE_ParseBoundary;
					}
				}
			    break;
			case STATE_ParseBoundary:
				if ((c == 10) || (c == 13)){
					muCrlfCount = 1;
					muParseState = STATE_SearchEndOfHeaderLine;
				}
				else{
					if (c != ' ')
						mBoundary += c;
				}
			    break;
			case STATE_CopyBody:
				uPos--;
				mBody.append(sBuffer + uPos, uLen - uPos);
				mbFinished = mBody.size() >= muContentLength;
				return true;
				
			case STATE_ProcessMultipartBodyStart:
				mStringParser.ConsumeCharSimple(c);
				muActBodyLength++;
				__uint8_t u;
				if (mStringParser.Found(u))
					muParseState = STATE_ProcessMultipartBody;
				mbFinished = muActBodyLength >= muContentLength;
				break;
			case STATE_ProcessMultipartBody:
				uPos--;
				ProcessMultipartBody(sBuffer + uPos, uLen - uPos);
				mbFinished = muActBodyLength >= muContentLength;
				return true;
		}
	}
	return true;
}

void HttpRequestParser::ProcessMultipartBody(char* sBuffer, __uint16_t uLen){
	if (muActBodyLength + 8 + mBoundary.size() < muContentLength){
		if (muActBodyLength + 8 + mBoundary.size() + uLen > muContentLength){
			__uint16_t u = muContentLength - (muActBodyLength + 8 + mBoundary.size());
			mBody.append(sBuffer, u);
			return;
		}
		mBody.append(sBuffer, uLen);
	}
	muActBodyLength+= uLen;
}
