#ifndef MAIN_WEBSERVER_H_
#define MAIN_WEBSERVER_H_

#include "freertos/FreeRTOS.h"
#include "openssl/ssl.h"

class Ufo;
class DisplayCharter;

class WebServer {
public:
	WebServer();
	virtual ~WebServer();

	void SetUfo(Ufo* pUfo) { mpUfo = pUfo; };
	void SetDisplayCharter(DisplayCharter* pDCLevel1, DisplayCharter* pDCLevel2){ mpDisplayCharterLevel1 = pDCLevel1; mpDisplayCharterLevel2 = pDCLevel2; };

	bool Start();

	void WebRequestHandler(int socket);

private:
	Ufo* mpUfo;
	DisplayCharter* mpDisplayCharterLevel1;
	DisplayCharter* mpDisplayCharterLevel2;

	SSL_CTX* mpSslCtx;

};

#endif /* MAIN_WEBSERVER_H_ */
