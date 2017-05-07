#include "freertos/FreeRTOS.h"
#include "httpResponse.h"
#include <esp_log.h>

constexpr char HttpResponse::HeaderContentTypeJson[];
constexpr char HttpResponse::HeaderContentTypeHtml[];
constexpr char HttpResponse::HeaderContentTypeBinary[];
constexpr char HttpResponse::HeaderNoCache[];

void HttpResponse::Init(int socket, __uint16_t uRetCode, bool bHttp11, bool bConnectionClose){
	PrivateInit(bHttp11, bConnectionClose);
	mSocket = socket;
	muRetCode = uRetCode;
}

void HttpResponse::Init(SSL* pSsl, __uint16_t uRetCode, bool bHttp11, bool bConnectionClose){
	PrivateInit(bHttp11, bConnectionClose);
	mpSsl = pSsl;
	muRetCode = uRetCode;
}

void HttpResponse::Init(int socket, bool bHttp11, bool bConnectionClose){
	PrivateInit(bHttp11, bConnectionClose);
	mSocket = socket;
	muRetCode = 200;
}

void HttpResponse::Init(SSL* pSsl, bool bHttp11, bool bConnectionClose){
	PrivateInit(bHttp11, bConnectionClose);
	mpSsl = pSsl;
	muRetCode = 200;
}

void HttpResponse::PrivateInit( bool bHttp11, bool bConnectionClose){
	mbHttp11 = bHttp11;
	mbConnectionClose = bConnectionClose;
	mHeaders.clear();
}

void HttpResponse::AddHeader(const char* sHeader){
	mHeaders.push_back(sHeader);
}

bool HttpResponse::Send(const char* sBody, __uint16_t uBodyLen){

	char sBuf[10];

	if (!SendInternal(mbHttp11 ? "HTTP/1.1 " : "HTTP/1.0 ", 9))
		return false;
	__uint8_t len = Number2String(muRetCode, sBuf);
	if (!SendInternal(sBuf, len))
		return false;
	const char* sData = GetResponseMsg(muRetCode, len);
	if (!SendInternal(sData, len))
			return false;

	std::list<std::string>::iterator it = mHeaders.begin();
	while (it != mHeaders.end()){
		if (!SendInternal(it->data(), it->size()))
			return false;
		if (!SendInternal("\r\n", 2))
			return false;
		it++;
	}
	if (!mbConnectionClose){
		if (!SendInternal("Content-Length: ", 16))
			return false;

		__uint8_t len = Number2String(uBodyLen, sBuf);
		if (!SendInternal(sBuf, len))
			return false;
		if (!SendInternal("\r\n\r\n", 4))
			return false;
	}
	else{
		if (!SendInternal("\r\n", 2))
			return false;
	}
	if (sBody && uBodyLen){
		if (!SendInternal(sBody, uBodyLen))
			return false;
	}
	else{
		if (!SendInternal("\r\n", 2))
			return false;
	}
	return true;
}

bool HttpResponse::SendInternal(const char* sData, __uint16_t uLen){

	if (mpSsl){
		return  SSL_write(mpSsl, sData, uLen) > 0;
	}
	return send(mSocket, sData, uLen, 0) == uLen;  //theoretically less bytes could be sent (without error)
};

const char* HttpResponse::GetResponseMsg(__uint16_t uRetCode, __uint8_t& ruLen){

	switch(uRetCode){
		case 200:
			ruLen = 5;
			return " OK\r\n";
		case 301:
			ruLen = 20;
			return " Moved Permanently\r\n";
		case 302:
			ruLen = 20;
			return " Found\r\n";
		case 304:
			ruLen = 15;
			return " Not Modified\r\n";
		case 401:
			ruLen = 15;
			return " Unauthorized\r\n";
		case 404:
			ruLen = 12;
			return " Not Found\r\n";
		case 500:
			ruLen = 24;
			return " Internal Server Error\r\n";
	}
	ruLen = 10;
	return " Unknown\r\n";
}

__uint8_t HttpResponse::Number2String(__uint16_t uNum, char* sBuf){
	char sHelp[10];
	__uint8_t uPos = 0;

	if (!uNum){
		sBuf[0] = '0';
		return 1;
	}

	while (uNum){
		sHelp[uPos++] = uNum % 10 + '0';
		uNum /= 10;
	}
	for (__uint8_t u=0 ; u<uPos ; u++){
		sBuf[u] = sHelp[uPos - u - 1];
	}
	return uPos;

}
