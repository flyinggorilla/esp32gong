#ifndef MAIN_WEBCLIENT_HPP_
#define MAIN_WEBCLIENT_HPP_

#include <stdio.h>
#include <list>
#include <string>
#include "UriParser.hpp"
#include "DownloadHandler.hpp"


class WebClient {
public:
	WebClient();
	virtual ~WebClient();

	bool HttpPrepareGet(std::string url, DownloadHandler* pOptionalDownloadHandler = NULL);
	bool HttpAddHeader(std::string& sHeader);
	bool HttpExecute();

	int exit_flag = 0;

	//void ConnectEvent(int errorcode);
	//void ReceiveEvent(char* data, size_t len);
	//void CloseEvent();
	//void ReplyEvent();
	UriParser& GetUri() { return mUri; }

private:
	DownloadHandler* mpDownloadHandler;

	UriParser mUri;
	std::list<std::string> mlRequestHeaders;

};



#endif /* MAIN_WEBCLIENT_HPP_ */
