#ifndef MAIN_WEBSERVER_HPP_
#define MAIN_WEBSERVER_HPP_

#include "freertos/FreeRTOS.h"


class WebServer {
public:
	WebServer();
	virtual ~WebServer();

	bool Start(__uint16_t port);

	void WebRequestHandler(int socket);

private:



};

#endif /* MAIN_WEBSERVER_HPP_ */
