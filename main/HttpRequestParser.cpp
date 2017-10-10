
#include "HttpRequestParser.h"
#include "DownAndUploadHandler.h"
#include "String.h"
#include "freertos/FreeRTOS.h"
#include <esp_log.h>

constexpr char HttpRequestParser::TAG[];

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
#if (DUMP_HEADER == 1)
	mHeader.clear();
	bAccumInHeader = false;
#endif
#if (EXTRA_PARSING == 1)
	mBoundaryName.clear();
	mBoundaryFilename.clear();
	bBoundaryValueIsAdjourned = false;
	muActBoundaryLength = 0;
	boundaryInstance.clear();
	muSizeOfBoundaryTail = 0;
#endif
}

bool HttpRequestParser::ParseRequest(char* sBuffer, __uint16_t uLen){

	__uint16_t uPos = 0;
#if (EXTRA_PARSING == 1)
	uint16_t u = 0;
#endif
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
#if (EXTRA_PARSING == 1)
								muParseState = STATE_ProcessBoundaryInstance;
								mStringParser.Init();
								boundaryInstance = "--"; //must be global
								boundaryInstance += mBoundary;
								boundaryInstance.toLowerCase();
								mStringParser.AddStringToParse(boundaryInstance.c_str());
#else
								muParseState = STATE_ProcessMultipartBodyHeaders;
								mStringParser.Init();
								mStringParser.AddStringToParse("filename=\"");
								mFilename.clear();
#endif
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

#if (EXTRA_PARSING == 0) //old cases
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
#endif //(EXTRA_PARSING == 0) //old cases


#if (EXTRA_PARSING == 1)
			case STATE_SearchEndOfBoundaryBody:
				if ((c != 10) && (c != 13)){
					uPos--;
					muParseState = STATE_ProcessBoundaryInstance;
					mStringParser.Init();
					mStringParser.AddStringToParse(boundaryInstance.c_str());
				} else {
#if (DUMP_HEADER == 1)
					mHeader += c;
#endif
					muActBoundaryLength++;
				}
				break;
			case STATE_ParseBoundaryBody:
		ESP_LOGD(TAG, "%s() STATE_ParseBoundaryBody mpActParam=%d bBoundaryValueIsAdjourned=%d", __func__, (int)mpActParam, (int)bBoundaryValueIsAdjourned);
				if (mpActParam && bBoundaryValueIsAdjourned) {
					//we are just waiting the value on the separate line
					if ((c == 10) || (c == 13)){
		ESP_LOGD(TAG, "%s() STATE_ParseBoundaryBody mpActParam: paramName=%s paramValue=%s", __func__, mpActParam->paramName.c_str(), mpActParam->paramValue.c_str());
						bBoundaryValueIsAdjourned = false;
						muCrlfCount = 1;
						muParseState = STATE_SearchEndOfBoundaryBody;
					} else {
						mpActParam->paramValue += c;
					}
#if (DUMP_HEADER == 1)
					mHeader += c;
#endif
					muActBoundaryLength++;
				} else {
					uPos--;
					muParseState = STATE_ProcessMultipartBodyStart2;
				}
				break;
			case STATE_ParseBoundaryName:
				if ((c == 10) || (c == 13)){
					muCrlfCount = 1;
					muParseState = STATE_SearchEndOfBoundaryLine;
				}
				else{
					if (c != ' ') {
						if (c == '"') {
							if (mBoundaryName.length() > 0) {
								//end of name
								muParseState = STATE_SearchBoundaryName;
								mParams.emplace_back();
								mpActParam = &mParams.back();
								mpActParam->paramName = mBoundaryName;
								mpActParam->paramValue = ""; //the value is placed on the separate line
								bBoundaryValueIsAdjourned = true;
		ESP_LOGD(TAG, "%s() STATE_ParseBoundaryName mpActParam: paramName=%s", __func__, mpActParam->paramName.c_str());
							} //else ignore open quote
						} else {
							mBoundaryName += c;
						}
					}
				}
#if (DUMP_HEADER == 1)
				mHeader += c;
#endif
				muActBoundaryLength++;
				break;
			case STATE_ParseBoundaryFilenameValue:
				if ((c == 10) || (c == 13)){
					muCrlfCount = 1;
					muParseState = STATE_SearchEndOfBoundaryLine;
				}
				else{
					if (c != ' ') {
						if (c == '"') {
							if (mBoundaryFilename.length() > 0) {
								//end of name
								muParseState = STATE_SearchBoundaryName;
								mParams.emplace_back();
								mpActParam = &mParams.back();
								mpActParam->paramName = "filename";
								mpActParam->paramValue = mBoundaryFilename;
		ESP_LOGD(TAG, "%s() STATE_ParseBoundaryFilenameValue mpActParam: paramName=%s paramValue=%s", __func__, mpActParam->paramName.c_str(), mpActParam->paramValue.c_str());
								bBoundaryValueIsAdjourned = false; //reset STATE_ParseBoundaryName flag because we have found 'filename'
							} //else ignore open quote
						} else {
							mBoundaryFilename += c;
						}
					}
				}
#if (DUMP_HEADER == 1)
				mHeader += c;
#endif
				muActBoundaryLength++;
				break;
			case STATE_SearchBoundaryName:
				if ((c == 10) || (c == 13)){
					muCrlfCount = 1;
					muParseState = STATE_SearchEndOfBoundaryLine;
				}
				else{
					if ((c != ';') && (c != ' ')){
						uint8_t u;
						mStringParser.ConsumeCharSimple(c);
						if (mStringParser.Found(u)){
							switch (u){
								case 0: //filename=
									mBoundaryFilename.clear();
									muParseState = STATE_ParseBoundaryFilenameValue;
									break;
								case 1: //name=
									mBoundaryName.clear();
									muParseState = STATE_ParseBoundaryName;
									break;
							}
						}
					}
				}
#if (DUMP_HEADER == 1)
				mHeader += c;
#endif
				muActBoundaryLength++;
				break;
			case STATE_CheckBoundaryData:
				if ((c == 10) || (c == 13) || (c==';')){
					uint8_t u;
					if (mStringParser.Found(u)){
						switch (u){
							case 0: //form-data
								muParseState = STATE_SearchBoundaryName;
								mStringParser.Init();
								mStringParser.AddStringToParse("filename="); //exactly in this order. If we will set 'name=' in first position than 'filename' will be found as 'name'
								mStringParser.AddStringToParse("name=");
								break;
						}
					}
				}
				else{
					if (!mStringParser.ConsumeChar(c, true)) {
						muParseState = STATE_SkipRestOfBoundary;
					}
				}
#if (DUMP_HEADER == 1)
				mHeader += c;
#endif
				muActBoundaryLength++;
				break;
			case STATE_CheckBoundaryContent:
				if (c == ':'){
					uint8_t uFound;
					if (mStringParser.Found(uFound)){
						switch (uFound){
							case 0: //content-disposition
								muParseState = STATE_CheckBoundaryData;
								mStringParser.Init();
								mStringParser.AddStringToParse("form-data");
								break;
							case 1: //content-type
								muParseState = STATE_SkipRestOfBoundary;
								break;
						}
					}
					else
						muParseState = STATE_SkipRestOfBoundary;
				}
				else{
					if (!mStringParser.ConsumeChar(c)){
						if ((c == 10) || (c == 13))
							muParseState = STATE_SearchEndOfBoundaryLine;
						else
							muParseState = STATE_SkipRestOfBoundary;
					}
				}
#if (DUMP_HEADER == 1)
				mHeader += c;
#endif
				muActBoundaryLength++;
				break;
			case STATE_SearchEndOfBoundaryLine:
				if ((c != 10) && (c != 13)){
					muParseState = STATE_CheckBoundaryContent;
					mStringParser.Init();
					mStringParser.AddStringToParse("content-disposition");
					mStringParser.AddStringToParse("content-type");
					uPos--;
				}
				else{
					if (++muCrlfCount == 4){
						muParseState = STATE_ParseBoundaryBody;
					}
#if (DUMP_HEADER == 1)
					mHeader += c;
#endif
					muActBoundaryLength++;
				}
				break;
			case STATE_SkipRestOfBoundary:
				if ((c == 10) || (c == 13)){
					muCrlfCount = 1;
					muParseState = STATE_SearchEndOfBoundaryLine;
				}
#if (DUMP_HEADER == 1)
				mHeader += c;
#endif
				muActBoundaryLength++;
				break;
			case STATE_ProcessBoundaryInstance:
				if ((c == 10) || (c == 13)){
					muCrlfCount = 1;
					muParseState = STATE_SearchEndOfBoundaryLine;
				}else{
					if (!mStringParser.ConsumeChar(c)){
						muParseState = STATE_SkipRestOfBoundary;
					}
				}
#if (DUMP_HEADER == 1)
				mHeader += c;
#endif
				muActBoundaryLength++;
				break;
			case STATE_ProcessMultipartBodyStart2:
				uPos--;
				muParseState = STATE_ProcessMultipartBody2;
				muActBoundaryLength += 42+4; //42 - boundary-- at the end, 4-<crlf><crlf>
				if (mpUploadHandler && (muActBoundaryLength <= muContentLength)) {
					unsigned int fileLength = muContentLength - muActBoundaryLength;
					if (!mpUploadHandler->OnReceiveBegin(mUrl, fileLength)){
						ESP_LOGD(TAG, "%s() OnReceiveBegin failed, fileLength=%u", __func__, fileLength);
						return SetError(6), false;
					}
				}
				mbFinished = muActBoundaryLength >= muContentLength;
				if (mbFinished) {
					ESP_LOGD(TAG, "%s() STATE_ProcessMultipartBodyStart2 mbFinished=true", __func__);
				}
				break;
			case STATE_ProcessMultipartBody2:
				uPos--;
				//uLen -= uPos;
				//ESP_LOGD(TAG, "%s() STATE_ProcessMultipartBody2 uPos=%u uLen=%u muActBoundaryLength=%u muContentLength=%u", __func__, uPos, uLen, muActBoundaryLength, muContentLength);
				if (mpUploadHandler && (muActBoundaryLength <= muContentLength)) {
					u = uLen - uPos; //uLen;
					//if ((muActBoundaryLength + uLen) > muContentLength){
					if ((muActBoundaryLength + u) > muContentLength){
						u = muContentLength - muActBoundaryLength;
					}
					if (!ProcessMultipartBody(sBuffer + uPos, u)) {
						mpUploadHandler->OnReceiveEnd();
						ESP_LOGD(TAG, "%s() ProcessMultipartBody() failed, u=%u", __func__, u);
						return SetError(6), false;
					}
					muActBoundaryLength += u;
					uPos += u;
					ESP_LOGD(TAG, "%s() ProcessMultipartBody written u=%u rest=%u", __func__, u, muContentLength-muActBoundaryLength);

					if ((muActBoundaryLength >= muContentLength) && mpUploadHandler && !mbStoreUploadInBody) {
						mpUploadHandler->OnReceiveEnd();
						muSizeOfBoundaryTail = 42+4; //42 - boundary-- at the end, 4-<crlf><crlf>
#if (DUMP_HEADER == 1)
	if (mHeader.length() > 0) {
		ESP_LOGD(TAG, "%s() ============== mHeader begin", __func__);
		#if (CONFIG_LOG_DEFAULT_LEVEL>=4)
		printf("%s", mHeader.c_str());
		#endif
		ESP_LOGD(TAG, "%s() ============== mHeader end", __func__);
		mHeader = "";
	}
#endif
					}
				}

				u = uLen - uPos;
				if (u > 0) {
					//tail chars
		ESP_LOGD(TAG, "%s() ============== Tail begin u=%u muSizeOfBoundaryTail=%u", __func__, u, muSizeOfBoundaryTail);
		#if (CONFIG_LOG_DEFAULT_LEVEL>=4)
					char curC;
					for (uint16_t i = uPos; i < uLen; i++) {
						curC = sBuffer[i];
						if (curC < ' ') {
							if ((curC != '\r') && (curC != '\n')) curC = '_'; //unprintable chars -> '_'
						}
						printf("%c", curC);
					}
					printf("\r\n");
		#endif
		ESP_LOGD(TAG, "%s() ============== Tail end", __func__);
					if (u <= muSizeOfBoundaryTail) {
						muSizeOfBoundaryTail -= u;
					} else {
						muSizeOfBoundaryTail = 0;
					}
				}


				mbFinished = (muSizeOfBoundaryTail == 0) && (muActBoundaryLength >= muContentLength);
				if (mbFinished) {
					ESP_LOGD(TAG, "%s() STATE_ProcessMultipartBody2 mbFinished=true", __func__);
				}
				return true;
#endif //(EXTRA_PARSING == 1)

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