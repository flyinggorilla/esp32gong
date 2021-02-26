
#include "HttpRequestParser.h"
#include "DownAndUploadHandler.h"
#include "EspString.h"
#include "freertos/FreeRTOS.h"
#include <esp_log.h>


HttpRequestParser::HttpRequestParser(int socket) {
	mSocket = socket;

	Init(NULL);
}

HttpRequestParser::~HttpRequestParser() {
}

void HttpRequestParser::Init(std::list<UploadHandler>* pUploadHandlerList){
	Clear();

	mpUploadHandlerList = pUploadHandlerList;
	mbStoreUploadInBody = false;

	muError = 0;
	mbParseFormBody = false;
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

		/*if (c >= 32 && c < 126) {
			putchar(c);
		} else if (c == 13) {
			printf("<CR>");
		} else if (c == 10) {
			printf("<LF>");
		} else {
			putchar('_');
		}*/

		//ESP_LOGD("HttpRequestParser", "St: %d, Char: %c", muParseState, c);	
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
					uPos--;
				}
				else{
					if (++muCrlfCount == 4){
						if (mbIsGet){
							mbFinished = true;
							return true;
						}
						else{
							if (mBoundary.length()){
								muParseState = STATE_ProcessMultipartBodyHeaders;
								mStringParser.Init();
								mStringParser.AddStringToParse("filename=\"");
								mFilename.clear();
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
				uPos--;
				mBody.concat(sBuffer + uPos, uLen - uPos);
				mbFinished = mBody.length() >= muContentLength;
				return true;

			case STATE_ProcessMultipartBodyHeaders:
				mStringParser.ConsumeCharSimple(c);
				muActBodyLength++;
				__uint8_t u;
				if (mStringParser.Found(u)) {
					muParseState = STATE_ProcessMultipartBodyFilename;
				}
				break;

			case STATE_ProcessMultipartBodyFilename:
				muActBodyLength++;
				if (c == '"') {
					muParseState = STATE_ProcessMultipartBodyStart;
					mStringParser.Init();
					mStringParser.AddStringToParse("\r\n\r\n");
				} else {
					mFilename += c;
				}
				break;
				
			case STATE_ProcessMultipartBodyStart:
				mStringParser.ConsumeCharSimple(c);
				muActBodyLength++;
				if (mStringParser.Found(u)){
					muParseState = STATE_ProcessMultipartBody;
					mpUploadHandler = NULL;
					std::list<UploadHandler>::iterator it = mpUploadHandlerList->begin();
					while (it != mpUploadHandlerList->end()){
						if ((*it).mUrl == mUrl ){
							mpUploadHandler = (*it).mpUploadHandler;
							mbStoreUploadInBody = !mpUploadHandler;
							break;
						}
						it++;
					}

					if (mpUploadHandler) {
						if(!mpUploadHandler->OnReceiveBegin(mFilename, muContentLength)) {
							return SetError(5), false;
						}
					}
				}
				mbFinished = muActBodyLength >= muContentLength;
				break;
			case STATE_ProcessMultipartBody:
				uPos--;
				uLen -= uPos;

				if (muActBodyLength + 8 + mBoundary.length() < muContentLength){
					if (muActBodyLength + 8 + mBoundary.length() + uLen > muContentLength){
						// end of multi-part content reached, substracting boundary
						__uint16_t u = muContentLength - (muActBodyLength + 8 + mBoundary.length());
						if (!ProcessMultipartBody(sBuffer + uPos, u))
							return SetError(6), false;
					}
					else{
						if (!ProcessMultipartBody(sBuffer + uPos, uLen))
							return SetError(6), false;
					}	
				}
				muActBodyLength+= uLen;
				mbFinished = muActBodyLength >= muContentLength;
				if (mbFinished && mpUploadHandler && !mbStoreUploadInBody)
					mpUploadHandler->OnReceiveEnd();
				return true;
		}
	}
	return true;
}

bool HttpRequestParser::ProcessMultipartBody(char* sBuffer, __uint16_t uLen){
	if (mbStoreUploadInBody){
		mBody.concat(sBuffer, uLen);
		return true;
	}
	if (mpUploadHandler)
		return mpUploadHandler->OnReceiveData(sBuffer, uLen);
	return false;
}