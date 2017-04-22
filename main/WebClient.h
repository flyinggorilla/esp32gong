#ifndef MAIN_WEBCLIENT_H_
#define MAIN_WEBCLIENT_H_

#include <stdio.h>
#include <list>
#include <string>

#include "DownloadHandler.h"
#include "Url.h"

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
	bool HttpExecuteSecure();
};



#endif /* MAIN_WEBCLIENT_H_ */
