#ifndef MAIN_HTTPRESPONSE_H_
#define MAIN_HTTPRESPONSE_H_

#include "String.h"
#include <list>
#include <lwip/sockets.h>
#include "openssl/ssl.h"


class HttpResponse {
public:
	HttpResponse() { mpSsl = NULL; };
	virtual ~HttpResponse() {};

	void Init(int socket, bool bHttp11, bool bConnectionClose);
	void Init(int socket, __uint16_t uRetCode, bool bHttp11, bool bConnectionClose);
	void Init(SSL* pSsl, bool bHttp11, bool bConnectionClose);
	void Init(SSL* pSsl, __uint16_t uRetCode, bool bHttp11, bool bConnectionClose);

	void SetRetCode(__uint16_t uRetCode) { muRetCode = uRetCode; };
	void AddHeader(const char* sHeader);
	void AddHeader(const char* sName, __uint16_t  uValue);

	bool Send(const char* sBody, __uint16_t uBodyLen);
	bool Send(String& sResponse) { return Send(sResponse.c_str(), sResponse.length()); };
	bool Send() { return Send(NULL, 0); };

public:
	constexpr static char HeaderContentTypeJson[] = "content-type: text/json";
	constexpr static char HeaderContentTypeHtml[] = "content-type: text/html";
	constexpr static char HeaderContentTypeBinary[] = "content-type: application/octet-stream";
	constexpr static char HeaderNoCache[] = "cache-control: private, max-age=0, no-cache, no-store";

private:
	void PrivateInit( bool bHttp11, bool bConnectionClose);
	bool SendInternal(const char* sData, __uint16_t uLen);
	const char* GetResponseMsg(__uint16_t uRetCode, __uint8_t& ruLen);
	__uint8_t Number2String(__uint16_t uNum, char* sBuf);

private:
	int mSocket;
	SSL* mpSsl;
	__uint16_t muRetCode;
	bool mbHttp11;
	bool mbConnectionClose;
	std::list<String> mHeaders;

};

#endif
