#ifndef MAIN_WEBSERVER_H_
#define MAIN_WEBSERVER_H_

#include "freertos/FreeRTOS.h"


class WebServer {
public:
	WebServer();
	virtual ~WebServer();

	bool Start(__uint16_t port);

	void WebRequestHandler(int socket);

private:



};

#endif /* MAIN_WEBSERVER_H_ */
