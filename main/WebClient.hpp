/*
 * WebClient.h
 *
 *  Created on: 09.04.2017
 *      Author: bernd
 */

#ifndef MAIN_WEBCLIENT_HPP_
#define MAIN_WEBCLIENT_HPP_

#include "mongoose.h"

class DownloadHandler;

class WebClient {
public:
	WebClient();
	virtual ~WebClient();
	bool request(const char* url, DownloadHandler* downloadHandler = NULL);

	int exit_flag = 0;

	void ConnectEvent(int errorcode);
	void ReceiveEvent(char* data, size_t len);
	void CloseEvent();
	void ReplyEvent();

private:
	struct mg_mgr mgr;
	DownloadHandler* downloadHandler;



};


class DownloadHandler {
public:
	//DownloadHandler();
	//virtual ~DownloadHandler();
	virtual void OnReceiveBegin() =0;
	virtual void OnReceiveEnd() =0;
	virtual bool OnReceiveData(char* buf, int len) =0; // =0 means pure virtual; must override

};
#endif /* MAIN_WEBCLIENT_HPP_ */
