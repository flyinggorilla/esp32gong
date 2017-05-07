#ifndef MAIN_DYNAMICREQUESTHANDLER_H_
#define MAIN_DYNAMICREQUESTHANDLER_H_

#include "UrlParser.h"
#include "HttpResponse.h"
#include <string>
#include <list>

class DynamicRequestHandler {
public:
	DynamicRequestHandler();
	virtual ~DynamicRequestHandler();

	bool HandleApiRequest(std::list<TParam>& params, HttpResponse& rResponse);
	bool HandleApiListRequest(std::list<TParam>& params, HttpResponse& rResponse);
	bool HandleApiEditRequest(std::list<TParam>& params, HttpResponse& rResponse);
	bool HandleInfoRequest(std::list<TParam>& params, HttpResponse& rResponse);
	bool HandleConfigRequest(std::list<TParam>& params, HttpResponse& rResponse);
	bool HandleFirmwareRequest(std::list<TParam>& params, HttpResponse& response);

	void CheckForRestart();

private:
	bool mbRestart;
};

#endif /* MAIN_DYNAMICREQUESTHANDLER_H_ */
