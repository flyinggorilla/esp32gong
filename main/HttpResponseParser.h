#ifndef MAIN_HTTPREQUESTPARSER_HPP_
#define MAIN_HTTPREQUESTPARSER_HPP_

//#include <string>
#include <list>

#include "DownloadHandler.h"
#include "String.h"
#include "StringParser.h"


class HttpResponseParser {
public:
	HttpResponseParser();
	virtual ~HttpResponseParser();

	/*
	 * Initializes Http Response Parser which sends downloaded content into internal buffer or into DownloadHandler
	 * @param pDownloadHandler
	 * 			- if NULL, internal dynamic buffer will be used to store message content, which can be accessed with GetBody()
	 * 			- if set, the message body data stream will be directly forwarded to the DownloadHandler implementation
	 * 	@param maxBodyBufferSize
	 * 			- protects from allocating too much memory to store the message body.
	 * 			  default is 16kB max message body size when not using DownloadHandlers.
	 * 			  optionally use Clear() to release message body memory.
	 * 			- irrelevant if DownloadHandler is set
	 */
	void Init(DownloadHandler* pDownloadHandler, unsigned int maxBodyBufferSize = 16*1024);


	/*
	 * feed parser with stream data, in a loop until ResponseFinised() is true
	 *
	 * example:
	 * @code{cpp}
	 * while(!ResponseFinished()) {
	 * 		len = socket_receive(so, buf, sizeof(buf))
	 * 		// add error checking
	 * 		if (!ParseResponse(buf, len)) {
	 * 			return false;
	 * 		}
	 * 	}
	 * 	@endcode
	 */
	bool ParseResponse(char* sBuffer, unsigned int uLen);
	bool ResponseFinished() { return mbFinished; };

	bool IsHttp11() 		{ return mbHttp11; };
	bool IsConnectionClose(){ return mbConClose; };
	String& GetBody()  { return mBody; };
	String& GetContentType()  { return msContentType; };
	unsigned int GetContentLength() { return muActualContentLength; }
	unsigned short GetStatusCode() { return muStatusCode; }
	String& GetRedirectLocation() { return msLocation; }


	short GetError()  	{ return muError; };

private:
	void InternalInit(DownloadHandler* pDownloadHandler, unsigned int maxBodyBufferSize);
	void SetError(short u) { muError = u; };

private:
	String mBody;
	unsigned int muContentLength;
	unsigned int muActualContentLength;
	unsigned int muMaxBodyBufferSize;

	bool mbFinished;
	bool mbHttp11;
	bool mbConClose;
	bool mbContentLength;
	unsigned short muStatusCode;
	String msContentType;
	String msLocation;
	DownloadHandler* mpDownloadHandler = NULL;
	uint8_t muParseState;
	StringParser mStringParser;
	uint8_t muCrlfCount;
	uint8_t muError;
};



#endif /* MAIN_HTTPREQUESTPARSER_HPP_ */
