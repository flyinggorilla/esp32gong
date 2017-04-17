/*
 * WebServerOld.h
 *
 *  Created on: 10.04.2017
 *      Author: bernd
 */

#ifndef MAIN_WebServerOld_HPP_
#define MAIN_WebServerOld_HPP_

#define CONFIG_TCPIP_LWIP 1

#include "mongoose.h"

class WebServerOld {
public:
	WebServerOld();
	virtual ~WebServerOld();
	void start();

public:
	static char* createInfoJson();
	static const char* mgEventToString(int ev);
	static char* mgStrToStr(struct mg_str mgStr);
	void Restart(int seconds);

private:

};

#endif /* MAIN_WebServerOld_HPP_ */
