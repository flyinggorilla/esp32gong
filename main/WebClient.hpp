#ifndef MAIN_WEBCLIENT_HPP_
#define MAIN_WEBCLIENT_HPP_

#include <stdio.h>
#include <list>
#include <string>
#include "DownloadHandler.hpp"
#include "Url.hpp"

class WebClient {
public:
	WebClient();
	virtual ~WebClient();

	bool HttpPrepareGet(Url* pUrl, DownloadHandler* pOptionalDownloadHandler = NULL);
	bool HttpAddHeader(std::string& sHeader);
	bool HttpExecute();

private:
	DownloadHandler* mpDownloadHandler;
	Url* mpUrl = NULL;
	std::list<std::string> mlRequestHeaders;
};



#endif /* MAIN_WEBCLIENT_HPP_ */
