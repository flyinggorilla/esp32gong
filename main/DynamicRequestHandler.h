#ifndef MAIN_DYNAMICREQUESTHANDLER_H_
#define MAIN_DYNAMICREQUESTHANDLER_H_

#include "UrlParser.h"
#include "HttpResponse.h"
#include <list>

class DynamicRequestHandler {
public:
	DynamicRequestHandler();
	virtual ~DynamicRequestHandler();

	bool HandleApiRequest(std::list<TParam>& params, HttpResponse& rResponse);
	bool HandleInfoRequest(std::list<TParam>& params, HttpResponse& rResponse);
	bool HandleConfigRequest(std::list<TParam>& params, HttpResponse& rResponse);
	bool HandleSrvConfigRequest(std::list<TParam>& params, HttpResponse& rResponse);
	bool HandleFirmwareRequest(std::list<TParam>& params, HttpResponse& response);
	bool HandleCheckFirmwareRequest(std::list<TParam>& params, HttpResponse& response);

	bool ShouldRestart() { return mbRestart; }

private:

	bool mbRestart;
};

#endif /* MAIN_DYNAMICREQUESTHANDLER_H_ */
