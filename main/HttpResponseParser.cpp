#include "HttpResponseParser.hpp"

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include <algorithm>
#include <esp_log.h>

//TEMP
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STATE_HttpType				0
#define STATE_StatusCode			1
#define STATE_StatusMessage			2
#define STATE_SearchEndOfHeaderLine 3
#define STATE_SkipHeader			4
#define STATE_CheckHeaderName		5
#define STATE_CheckHeaderValue		6
#define STATE_ReadContentLength		7
#define STATE_ReadContentType		8
#define STATE_CopyBody				9

#define ERROR_HTTPRESPONSE_NOVALIDHTTP 1

#define HTTPRESPONSEBUFFER_MAX_BYTESRECEIVE 32*1024

static const char LOGTAG[] = "HttpResponseParser";

HttpResponseParser::HttpResponseParser() {
	Init();
}

HttpResponseParser::~HttpResponseParser() {
}

void HttpResponseParser::Init(DownloadHandler* pOptionalDownloadHandler){
	Clear();

	muError = 0;
	mbFinished = false;
	mbConClose = true;
	mbContentLength = false;
	mpDownloadHandler = pOptionalDownloadHandler;
	if (mpDownloadHandler) {
		mBody = "<DOWNLOADHANDLER_IS_SET>";
	}
	muContentLength = 0;
	muActualContentLength = 0;
	muStatusCode = 0;
	muParseState = STATE_HttpType;
	mStringParser.Init();
	mStringParser.AddStringToParse("http/1.0");
	mStringParser.AddStringToParse("http/1.1");
}

void HttpResponseParser::Clear(){
	//mUrl.clear();
	//mParams.clear();
	mBody.clear();
}


bool HttpResponseParser::ParseResponse(char* sBuffer, unsigned int uLen){

	unsigned int uPos = 0;
	while (uPos < uLen){
		char c = sBuffer[uPos];
		uPos++;

		switch (muParseState){

			case STATE_HttpType:
				if (c == ' '){
					uint8_t uFound;
					if (!mStringParser.Found(uFound))
						return SetError(1), false;
					mbHttp11 = uFound ? true : false;
					muParseState = STATE_StatusCode;
				}
				else{
					if (!mStringParser.ConsumeChar(c))
						return SetError(2), false;
				}
				break;

			case STATE_StatusCode:
				if (c == ' ') {
					mStringParser.Init();
					muParseState = STATE_StatusMessage;
				}
				else{
					if ((c >= '0') && (c<= '9')){
						muStatusCode *= 10;
						muStatusCode += c - '0';
					}
				}
				break;


			case STATE_StatusMessage:
				if ((c == 10) || (c == 13)){
					muCrlfCount = 1;
					muParseState = STATE_SearchEndOfHeaderLine;
				}

			case STATE_SearchEndOfHeaderLine:
				if ((c != 10) && (c != 13)){
					muParseState = STATE_CheckHeaderName;
					mStringParser.Init();
					mStringParser.AddStringToParse("connection");
					mStringParser.AddStringToParse("content-length");
					mStringParser.AddStringToParse("content-type");
				}
				else{
					if (++muCrlfCount == 4){
						if (mbContentLength && !muContentLength) {
							mbFinished = true;
							return true;
						}
						else
							muParseState = STATE_CopyBody;
							if (mpDownloadHandler && mbFinished) {
								if(!mpDownloadHandler->OnReceiveBegin()) {
									mbFinished = true;
									return false;
								}
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
					uint8_t uFound;
					if (mStringParser.Found(uFound)){
						if (!uFound){
							muParseState = STATE_CheckHeaderValue;
							mStringParser.Init();
							mStringParser.AddStringToParse("close");
							mStringParser.AddStringToParse("keep-alive");
						}
						else if (uFound == 1) {
							muParseState = STATE_ReadContentLength;
							muContentLength = 0;
							mbContentLength = true;
						} else {
							muParseState = STATE_ReadContentType;
							msContentType.clear();
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
					muCrlfCount = 1;
					uint8_t u;
					if (mStringParser.Found(u)){
						mbConClose = u ? false : true;
					}
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

			case STATE_ReadContentType:
				if ((c == 10) || (c == 13)){
					muCrlfCount = 1;
					muParseState = STATE_SearchEndOfHeaderLine;
				}
				else{
					msContentType += c;
				}
				break;

			case STATE_CopyBody:
				//fixup uPos which was already incremented at the beginning of this method
				uPos--;

				if (mbContentLength && !muContentLength)
					return SetError(7), false;
				if (uPos < uLen){
					size_t size = uLen - uPos;
					muActualContentLength += size;
					if (mpDownloadHandler) {
						if(!mpDownloadHandler->OnReceiveData((char*)sBuffer[uPos], size)) {
							mbFinished = true;
							return false;
						}
					} else {
						// skip data if there is more than HTTPRESPONSEBUFFER_MAX_BYTESRECEIVE bytes
						int appendSize = HTTPRESPONSEBUFFER_MAX_BYTESRECEIVE - mBody.length();
						appendSize = appendSize < size ? appendSize : size;
						if (appendSize < 0) {
							return  SetError(8), false;
						}
						mBody.append(&sBuffer[uPos], appendSize);
						ESP_LOGI(LOGTAG, "BODY:<%s>", mBody.c_str());
					}
					mbFinished = muActualContentLength >= muContentLength;
					if (mpDownloadHandler && mbFinished) {
						mpDownloadHandler->OnReceiveEnd();
					}
					return true;
				}
				return true;

		}
	}
	return true;
}
