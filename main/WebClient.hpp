#ifndef MAIN_WEBCLIENT_HPP_
#define MAIN_WEBCLIENT_HPP_

#include <stdio.h>
#include <list>
#include <string>
#include "UriParser.hpp"
#include "DownloadHandler.hpp"

struct TParam{
	std::string paramName;
	std::string paramValue;
};

class WebClient {
public:
	WebClient();
	virtual ~WebClient();

	bool HttpPrepareGet(Url& url, DownloadHandler* pOptionalDownloadHandler = NULL);
	bool HttpAddHeader(std::string& sHeader);
	bool HttpExecute();

private:
	DownloadHandler* mpDownloadHandler;
	Url& mUrl;
	std::list<std::string> mlRequestHeaders;
};



#endif /* MAIN_WEBCLIENT_HPP_ */
