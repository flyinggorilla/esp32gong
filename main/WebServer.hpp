/*
 * WebServer.h
 *
 *  Created on: 10.04.2017
 *      Author: bernd
 */

#ifndef MAIN_WEBSERVER_HPP_
#define MAIN_WEBSERVER_HPP_

#define CONFIG_TCPIP_LWIP 1

#include "mongoose.h"

class WebServer {
public:
	WebServer();
	virtual ~WebServer();
	void start();

public:
	static char* createInfoJson();
	static const char* mgEventToString(int ev);
	static char* mgStrToStr(struct mg_str mgStr);
	void Restart(int seconds);

private:

};

#endif /* MAIN_WEBSERVER_HPP_ */
