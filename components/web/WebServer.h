#ifndef MAIN_WEBSERVER_H_
#define MAIN_WEBSERVER_H_

#include "freertos/FreeRTOS.h"
#include "openssl/ssl.h"
#include <list>

class HttpRequestParser;
class HttpResponse;
class DownAndUploadHandler;
class UploadHandlerList;
class UploadHandler;
class String;

class WebServer {
public:
	WebServer();
	virtual ~WebServer();

	bool Start(__uint16_t port, bool useSsl, String* pCertificate);

	/* @brief: registers a DownAndUploadHandler object to handle HTTP POST based uploads for specific URL paths
	 * @param: url - relative path e.g. "/upload" that should be used for uploading with associated uploadhandler
	 * @param: pUploadHandler - uploadhandler (e.g. firmware, filesystem, ...) 
	 * 							NULL to store uploads in message body of request parser
	 */  
	void AddUploadHandler(String url, DownAndUploadHandler* pUploadHandler);

	void WebRequestHandler(int socket, int conCount);
	bool WaitForData(int socket, __uint8_t timeoutS);

	__uint8_t GetConcurrentConnections();
	void SignalConnection();
	void SignalConnectionExit();

	void EnterCriticalSection();
	void LeaveCriticalSection();

	//void AddUploadHandler(String& sUrl, DownAndUploadHandler* pUploadHandler);

	// override this method to set an Upload handler depending on URL
	virtual DownAndUploadHandler* HandleUploadRequest(String &sUrl) { return NULL; };

	// override this method to handle requests
	virtual bool HandleRequest(HttpRequestParser& httpParser, HttpResponse& httpResponse) = 0;


private:
	SSL_CTX* mpSslCtx;

	portMUX_TYPE myMutex;
	bool mbFree;
	__uint8_t muConcurrentConnections;

	std::list<UploadHandler> mUploadHandlerList;
	


};

#endif /* MAIN_WEBSERVER_H_ */
