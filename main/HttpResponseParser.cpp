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
#define STATE_CopyBody				9

#define ERROR_HTTPRESPONSE_NOVALIDHTTP 1

static const char LOGTAG[] = "HttpResponseParser";

HttpResponseParser::HttpResponseParser() {

}

HttpResponseParser::~HttpResponseParser() {
}

void HttpResponseParser::Init(DownloadHandler* pDownloadHandler, unsigned int maxBodyBufferSize) {
	mpDownloadHandler = pDownloadHandler;
	if (mpDownloadHandler) {
		mBody = "<DOWNLOADHANDLER_IS_SET>";
	}
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

bool HttpResponseParser::ParseResponse(char* sBuffer, unsigned int uLen) {
	// when uLen == 0, then connection is closed
	if (uLen == 0) {
		mbFinished = true;
		if (mpDownloadHandler)
			mpDownloadHandler->OnReceiveEnd();
		else
			ESP_LOGI(LOGTAG, "BODY:<%s>", mBody.c_str());
		ESP_LOGI(LOGTAG, "CONNECTION CLOSED: RECEIVED: %u", muActualContentLength);
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
					return SetError(1), false;
				mbHttp11 = uFound ? true : false;
				muParseState = STATE_StatusCode;
			} else {
				if (!mStringParser.ConsumeChar(c))
					return SetError(2), false;
			}
			break;

		case STATE_StatusCode:
			if (c == ' ') {
				mStringParser.Init();
				muParseState = STATE_StatusMessage;
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
			} else {
				if (++muCrlfCount == 4) {
					if (mbContentLength && !muContentLength) {
						mbFinished = true;
						return true;
					} else
					ESP_LOGI(LOGTAG, "HTTP: http/%s, status=%hu", mbHttp11 ? "1.1" : "1.0", muStatusCode);
					ESP_LOGI(LOGTAG, "HEADER: connection: %s",
							mbConClose ? "<close-connection header set>" : "<dont close connection>");
					ESP_LOGI(LOGTAG, "HEADER: content-type: %s", msContentType.c_str());
					ESP_LOGI(LOGTAG, "HEADER: content-length: %u %s", muContentLength,
							mbContentLength ? "" : "<no header set>");

					muParseState = STATE_CopyBody;
					if (mpDownloadHandler && mbFinished) {
						if (!mpDownloadHandler->OnReceiveBegin()) {
							mbFinished = true;
							return false;
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
					} else {
						muParseState = STATE_ReadContentType;
						msContentType.clear();
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

		case STATE_CopyBody:
			//fixup uPos which was already incremented at the beginning of this method
			uPos--;

			if (mbContentLength && !muContentLength)
				return SetError(3), false;

			if (uPos < uLen) {
				size_t size = uLen - uPos;
				muActualContentLength += size;
				if (mpDownloadHandler) {
					if (!mpDownloadHandler->OnReceiveData(&sBuffer[uPos], size)) {
						mbFinished = true;
						return false;
					}
				} else {
					// skip data if there is more than muMaxBodyBufferSize bytes
					int appendSize = muMaxBodyBufferSize - mBody.length();
					appendSize = appendSize < size ? appendSize : size;
					if (appendSize < 0) {
						return SetError(4), false;
					}
					mBody.append(&sBuffer[uPos], appendSize);
				}
				if (mbContentLength) {
					mbFinished = muActualContentLength >= muContentLength;
				}

				if (mpDownloadHandler && mbFinished) {
					mpDownloadHandler->OnReceiveEnd();
				}

				if (mbFinished) {
					ESP_LOGI(LOGTAG, "RECEIVED: %u", muActualContentLength);
					if (!mpDownloadHandler)
						ESP_LOGI(LOGTAG, "BODY:<%s>", mBody.c_str());
				}

				return true;
			}
			return true;

		}
	}
	return true;
}
