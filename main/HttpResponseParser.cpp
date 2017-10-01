#include "HttpResponseParser.h"

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
#define STATE_ReadLocation			9
#define STATE_CopyBody			   10

#define ERROR_OK 									0
#define ERROR_HTTPRESPONSE_NOVALIDHTTP 				1
#define ERROR_DOWNLOADHANDLER_ONRECEIVEDATA_ABORT 	2
#define ERROR_DOWNLOADHANDLER_ONRECEIVEBEGIN_ABORT 	3
#define ERROR_BODYBUFFERTOOSMALL					4
#define ERROR_ZEROCONTENTLENGTH						5
#define ERROR_INVALIDHTTPTYPE						6
#define ERROR_HTTPTYPENOTDETECTED					7
#define ERROR_DOWNLOADHANDLER_ONRECEIVEEND_FAILED	8



static const char LOGTAG[] = "HttpResponseParser";

HttpResponseParser::HttpResponseParser() {

}

HttpResponseParser::~HttpResponseParser() {
}

void HttpResponseParser::Init(DownAndUploadHandler* pDownloadHandler, unsigned int maxBodyBufferSize) {
	mpDownloadHandler = pDownloadHandler;
	mBody.clear();
	msLocation.clear();
	muContentLength = 0;
	muActualContentLength = 0;
	muMaxBodyBufferSize = maxBodyBufferSize;
	muStatusCode = 0;
	mbHttp11 = false;

	mbContentLength = false;
	mbFinished = false;
	mbConClose = true;
	msContentType.clear();
	muCrlfCount = 0;
	muError = 0;
	muParseState = STATE_HttpType;
	mStringParser.Init();
	mStringParser.AddStringToParse("http/1.0");
	mStringParser.AddStringToParse("http/1.1");

}

void HttpResponseParser::Clear(){
	mBody.clear();
	msLocation.clear();
	msContentType.clear();
}

bool HttpResponseParser::ParseResponse(char* sBuffer, unsigned int uLen) {

	// when uLen == 0, then connection is closed
	if (uLen == 0) {
		mbFinished = true;
		if (mpDownloadHandler) {
			if (!mpDownloadHandler->OnReceiveEnd()) {
				return SetError(ERROR_DOWNLOADHANDLER_ONRECEIVEEND_FAILED), false;
			}
		} else {
			ESP_LOGD(LOGTAG, "BODY:<%s>", mBody.c_str());
		}
		ESP_LOGD(LOGTAG, "CONNECTION CLOSED: RECEIVED: %u", muActualContentLength);
	}

	unsigned int uPos = 0;
	while (uPos < uLen) {
		char c = sBuffer[uPos];
		uPos++;

		switch (muParseState) {

		case STATE_HttpType:
			if (c == ' ') {
				uint8_t uFound;
				if (!mStringParser.Found(uFound))
					return SetError(ERROR_HTTPTYPENOTDETECTED), false;
				mbHttp11 = uFound ? true : false;
				muParseState = STATE_StatusCode;
				//ESP_LOGI(LOGTAG, "HTTPTYPE:");
			} else {
				if (!mStringParser.ConsumeChar(c))
					return SetError(ERROR_INVALIDHTTPTYPE), false;
			}
			break;

		case STATE_StatusCode:
			if (c == ' ') {
				mStringParser.Init();
				muParseState = STATE_StatusMessage;
				//ESP_LOGI(LOGTAG, "STATUSCODE: %u", muStatusCode);
			} else {
				if ((c >= '0') && (c <= '9')) {
					muStatusCode *= 10;
					muStatusCode += c - '0';
				}
			}
			break;

		case STATE_StatusMessage:
			if ((c == 10) || (c == 13)) {
				muCrlfCount = 1;
				muParseState = STATE_SearchEndOfHeaderLine;
			}
			break;

		case STATE_SearchEndOfHeaderLine:
			if ((c != 10) && (c != 13)) {
				muParseState = STATE_CheckHeaderName;
				mStringParser.Init();
				mStringParser.AddStringToParse("connection");
				mStringParser.AddStringToParse("content-length");
				mStringParser.AddStringToParse("content-type");
				mStringParser.AddStringToParse("location");
			} else {
				if (++muCrlfCount == 4) {
					if (mbContentLength && muContentLength == 0) {
						mbFinished = true;
						return SetError(ERROR_OK), true;
					}

					ESP_LOGD(LOGTAG, "HTTP: http/%s, status=%hu", mbHttp11 ? "1.1" : "1.0", muStatusCode);
					ESP_LOGD(LOGTAG, "HEADER: connection: %s",
							mbConClose ? "<close-connection header set>" : "<dont close connection>");
					ESP_LOGD(LOGTAG, "HEADER: content-type: %s", msContentType.c_str());
					ESP_LOGD(LOGTAG, "HEADER: content-length: %u %s", muContentLength,
							mbContentLength ? "" : "<no header set>");
					if (msLocation.length()) {
						ESP_LOGD(LOGTAG, "HEADER: location: %s", msLocation.c_str());
					}
					muParseState = STATE_CopyBody;
					if (mpDownloadHandler) {
						if (!mpDownloadHandler->OnReceiveBegin(muStatusCode, mbContentLength, muContentLength)) {
							ESP_LOGW(LOGTAG, "DownloadHandler signaled to abort download begin.");
							mbFinished = true;
							return SetError(ERROR_DOWNLOADHANDLER_ONRECEIVEBEGIN_ABORT), false;
						}
					}
				}
				break;
			}
			//No break here, fall through to STATE_CheckHeaderName!;

		case STATE_CheckHeaderName:
			if (c == ':') {
				uint8_t uFound;
				if (mStringParser.Found(uFound)) {
					if (uFound == 0) {
						muParseState = STATE_CheckHeaderValue;
						mStringParser.Init();
						mStringParser.AddStringToParse("close");
						mStringParser.AddStringToParse("keep-alive");
					} else if (uFound == 1) {
						muParseState = STATE_ReadContentLength;
						muContentLength = 0;
						mbContentLength = true;
					} else if (uFound == 2) {
						muParseState = STATE_ReadContentType;
						msContentType.clear();
					} else if (uFound == 3) {
						muParseState = STATE_ReadLocation;
						msLocation.clear();
					}
				} else
					muParseState = STATE_SkipHeader;
			} else {
				if (!mStringParser.ConsumeChar(c)) {
					if ((c == 10) || (c == 13))
						muParseState = STATE_SearchEndOfHeaderLine;
					else
						muParseState = STATE_SkipHeader;
				}
			}
			break;

		case STATE_SkipHeader:
			if ((c == 10) || (c == 13)) {
				muCrlfCount = 1;
				muParseState = STATE_SearchEndOfHeaderLine;
			}
			break;

		case STATE_CheckHeaderValue:
			if ((c == 10) || (c == 13)) {
				muCrlfCount = 1;
				uint8_t u;
				if (mStringParser.Found(u)) {
					mbConClose = u ? false : true;
				}
				muParseState = STATE_SearchEndOfHeaderLine;
			} else {
				if (!mStringParser.ConsumeChar(c))
					muParseState = STATE_SkipHeader;
			}
			break;

		case STATE_ReadContentLength:
			if ((c == 10) || (c == 13)) {
				muCrlfCount = 1;
				muParseState = STATE_SearchEndOfHeaderLine;
			} else {
				if ((c >= '0') && (c <= '9')) {
					muContentLength *= 10;
					muContentLength += c - '0';
				}
			}
			break;

		case STATE_ReadContentType:
			if ((c == 10) || (c == 13)) {
				muCrlfCount = 1;
				muParseState = STATE_SearchEndOfHeaderLine;
			} else {
				msContentType += c;
			}
			break;

		case STATE_ReadLocation:
			if ((c == 10) || (c == 13)) {
				muCrlfCount = 1;
				muParseState = STATE_SearchEndOfHeaderLine;
	 			mbFinished = true;
				return SetError(ERROR_OK), true; // drive immediate redirect
			} else {
				msLocation += c;
			}
			break;

		case STATE_CopyBody:
			//fixup uPos which was already incremented at the beginning of this method
			uPos--;

			if (mbContentLength && muContentLength == 0)
				return SetError(ERROR_ZEROCONTENTLENGTH), false;

			if (uPos < uLen) {
				size_t size = uLen - uPos;
				muActualContentLength += size;
				if (mpDownloadHandler) {
					if (!mpDownloadHandler->OnReceiveData(&sBuffer[uPos], size)) {
						ESP_LOGE(LOGTAG, "DownloadHandler aborted receiving data.");
						mbFinished = true;
						return SetError(ERROR_DOWNLOADHANDLER_ONRECEIVEDATA_ABORT), false;
					}
				} else {
					// skip data if there is more than muMaxBodyBufferSize bytes
					int appendSize = muMaxBodyBufferSize - mBody.length();
					appendSize = appendSize < size ? appendSize : size;
					if (appendSize < 0) {
						return SetError(ERROR_BODYBUFFERTOOSMALL), false;
					}
					mBody.concat(&sBuffer[uPos], appendSize);
				}
				if (mbContentLength) {
					mbFinished = muActualContentLength >= muContentLength;
				}

				if (mpDownloadHandler && mbFinished) {
					mpDownloadHandler->OnReceiveEnd();
				}

				if (mbFinished) {
					ESP_LOGD(LOGTAG, "RECEIVED: %u", muActualContentLength);
					if (!mpDownloadHandler)
						ESP_LOGD(LOGTAG, "BODY:<%s>", mBody.c_str());
				}
				return SetError(ERROR_OK), true;
			}
			return SetError(ERROR_OK), true;

		}
	}
	return SetError(ERROR_OK), true;
}
