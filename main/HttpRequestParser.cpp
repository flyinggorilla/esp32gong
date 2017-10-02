#include "HttpRequestParser.h"
#include "DownAndUploadHandler.h"
#include "String.h"
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

	mbStoreUploadInBody = false;
	muError = 0;
	mbParseFormBody = false;
	mbFinished = false;
	mbConClose = true;
	muPos = 0;
	muLen = 0;
	msBuffer = NULL;
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

bool HttpRequestParser::ParseRequestHeader(char* sBuffer, __uint16_t uLen){

	muPos = 0;
	muLen = uLen;
	msBuffer = sBuffer;

	while (muPos < muLen){
		char c = sBuffer[muPos];

		//ESP_LOGD("HttpRequestParser", "St: %d, Char: %c", muParseState, c);	
		muPos++;
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
				else{
					mUrlParser.ConsumeChar(c, mUrl, mpActParam);

					switch (mUrlParser.GetState()){
						case STATE_UrlComplete:
						case STATE_ParamComplete:
							mParams.emplace_back();
							mpActParam = &mParams.back();
							break;
					}
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
					muPos--;
				}
				else{
					if (++muCrlfCount == 4){
						if (mbIsGet){
							mbFinished = true;
							return true;
						}
						else{
							if (mBoundary.length()){
							    muParseState = STATE_ProcessMultipartBodyStart;
								mStringParser.Init();
								mStringParser.AddStringToParse("\r\n\r\n");
							}
							else if (mbParseFormBody){
								muParseState = STATE_ParseFormBody;
								mParams.emplace_back();
								mpActParam = &mParams.back();
							break;
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
							case 0: //connection
								muParseState = STATE_CheckHeaderValue;
								mStringParser.Init();
								mStringParser.AddStringToParse("close");
								mStringParser.AddStringToParse("keep-alive");
								break;
							case 1: //content-length
								muParseState = STATE_ReadContentLength;
								muContentLength = 0;
								break;
							case 2: //content-type
								muParseState = STATE_CheckHeaderValue;
								mStringParser.Init();
								mStringParser.AddStringToParse("multipart/form-data");
								mStringParser.AddStringToParse("application/x-www-form-urlencoded");
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
							switch (u){
								case 0: //multipart/form-data
									muParseState = STATE_SearchBoundary;
									mStringParser.Init();
									mStringParser.AddStringToParse("boundary=");
									break;
								case 1: //application/x-www-form-urlencoded
									mbParseFormBody = true;
									muParseState = STATE_SearchEndOfHeaderLine;
									break;
							}
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
		}
	}
	return true;
}


bool HttpRequestParser::ParseRequestBody(DownAndUploadHandler* pUploadHandler){
	
		while (muPos < muLen){
			char c = msBuffer[muPos];
	
			//ESP_LOGD("HttpRequestParser", "St: %d, Char: %c", muParseState, c);	
			muPos++;
			switch (muParseState){
				case STATE_ParseFormBody:
	
					mUrlParser.ConsumeChar(c, mUrl, mpActParam);
					muActBodyLength++;
	
					switch (mUrlParser.GetState()){
						case STATE_UrlComplete:
						case STATE_ParamComplete:
							mParams.emplace_back();
							mpActParam = &mParams.back();
							break;
					}
					mbFinished = muActBodyLength >= muContentLength;
					break;
	
				case STATE_CopyBody:
					muPos--;
					mBody.concat(msBuffer + muPos, muLen - muPos);
					mbFinished = mBody.length() >= muContentLength;
					return true;
					
				case STATE_ProcessMultipartBodyStart:
					mStringParser.ConsumeCharSimple(c);
					muActBodyLength++;
					__uint8_t u;
					if (mStringParser.Found(u)){
						muParseState = STATE_ProcessMultipartBody;
						if (!mbStoreUploadInBody) {
							if (!pUploadHandler || !pUploadHandler->OnReceiveBegin(mUrl, muContentLength)){
								return SetError(5), false;
							}
						}
					}
					mbFinished = muActBodyLength >= muContentLength;
					break;
				case STATE_ProcessMultipartBody:
					muPos--;
					muLen -= muPos;
	
					if (muActBodyLength + 8 + mBoundary.length() < muContentLength){
						if (muActBodyLength + 8 + mBoundary.length() + muLen > muContentLength){
							__uint16_t u = muContentLength - (muActBodyLength + 8 + mBoundary.length());
							if (!ProcessMultipartBody(msBuffer + muPos, u, pUploadHandler))
								return SetError(6), false;
						}
						else{
							if (!ProcessMultipartBody(msBuffer + muPos, muLen, pUploadHandler))
								return SetError(6), false;
						}	
					}
					muActBodyLength+= muLen;
					mbFinished = muActBodyLength >= muContentLength;
					if (mbFinished && pUploadHandler && !mbStoreUploadInBody)
						pUploadHandler->OnReceiveEnd();
					return true;
			}
		}
		return true;
	}
	


bool HttpRequestParser::ProcessMultipartBody(char* sBuffer, __uint16_t uLen, DownAndUploadHandler* pUploadHandler){
	if (mbStoreUploadInBody){
		mBody.concat(sBuffer, uLen);
		return true;
	}
	if (pUploadHandler)
		return pUploadHandler->OnReceiveData(sBuffer, uLen);
	return false;
}
