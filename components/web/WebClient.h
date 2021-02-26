#ifndef MAIN_WEBCLIENT_H_
#define MAIN_WEBCLIENT_H_

#include <cstdio>
#include <list>
#include "DownAndUploadHandler.h"
#include "Url.h"
#include "HttpResponseParser.h"
#include "EspString.h"

class WebClient {
public:
	WebClient();
	virtual ~WebClient();


	/*
	 * Prepares a HTTP request.
	 * Use e.g. AddRequestHeader and Certificate methods to prepare request before actually executing with Execute()
	 * @param pUrl
	 * 			- if NULL, internal dynamic buffer will be used to store message content, which can be accessed with GetBody()
	 * 			- if set, the message body data stream will be directly forwarded to the DownloadHandler implementation
	 * @return false if invalid URL was provided
	 */
	bool Prepare(Url* pUrl);

	void Clear();

	/*
	 * Adds HTTP headers to the request - you should have called Prepare() beforehand
	 * @param e.g. a header like "content-type: json/text"
	 */
	bool AddHttpHeader(String& sHeader);
	bool AddHttpHeaderCStr(const char* header);


	/*
	 * Sets a DownloadHandler for retrieving large responses that do not fit in memory.
	 * e.g. for retrieving a file to store into Flash memory or for streaming multimedia contents.
	 * @param pointer to instance of DownloadHandler class; you will need to create your own class and inherit from DownloadHandler
	 * @return
	 * 		- HTTP response status code
	 * 		- 0 on error
	 */
	void SetDownloadHandler(DownAndUploadHandler* pDownloadHandler);



	/*
	 * executes HTTP(S) GET request
	 * it stores response in internal dynamic memory in case no DownloadHandler is set
	 * optionally call SetMaxResponseDataSize() in case the default memory allocation limit of 16kB is too small
	 * optionally call Clear() to clear the buffer
	 * @return
	 * 		- HTTP response status code
	 * 		- 0 on error
	 */
	unsigned short HttpGet();


	/*
	 * executes HTTP(S) POST request
	 * it stores response in internal dynamic memory in case no DownloadHandler is set
	 * optionally call SetMaxResponseDataSize() in case the default memory allocation limit of 16kB is too small
	 * optionally call Clear() to clear the buffer
	 * @param POST data and size
	 * @return
	 * 		- HTTP response status code
	 * 		- 0 on error
	 */
	unsigned short HttpPost(const char* data, unsigned int size);


	/*
	 * executes HTTP(S) POST request
	 * it stores response in internal dynamic memory in case no DownloadHandler is set
	 * optionally call SetMaxResponseDataSize() in case the default memory allocation limit of 16kB is too small
	 * optionally call Clear() to clear the buffer
	 * @param POST data (can be binary too)
	 * @return
	 * 		- HTTP response status code
	 * 		- 0 on error
	 */
	unsigned short HttpPost(String& sData);

	/*
	 * in case the default max 16kB dynamic buffer limit is too small, you can increase the limit here.
	 * @param maxBodyBufferSize
	 * 			- protects from allocating too much memory to store the message body.
	 * 			  optionally use Clear() to release message body memory.
	 * @attention	for large data downloads it is recommended to use DownloadHandlers to stream the data without buffering
	 */
	void SetMaxResponseDataSize(unsigned int maxResponseDataSize) { muMaxResponseDataSize = maxResponseDataSize; }


	/*
	 * get a reference to response data
	 * use GetResponseData().data() for accessing binary data and GetResponseData().size() for its length
	 */
	String& GetResponseData() { return mHttpResponseParser.GetBody(); }


	/*
	 * @returns the HTTP response content-type. returns an empty string of no content-type header was set
	 */
	String& GetContentType() { return mHttpResponseParser.GetContentType(); }

	//TODO: verify server certificates / CA

private:
	HttpResponseParser mHttpResponseParser;
	DownAndUploadHandler* mpDownloadHandler = NULL;
	Url* mpUrl = NULL;
	std::list<String> mlRequestHeaders;
	const char* mpPostData = NULL;
	unsigned int muPostDataSize = 0;

	/*
	 * Note: The site "https://www.howsmyssl.com/a/check" is useful to test and experiment with TLS layer and CA Certificates
	 */
	unsigned short HttpExecuteSecure();
	unsigned int muMaxResponseDataSize;
	unsigned short HttpExecute();
	void PrepareRequest(String& sRequest);
};



#endif /* MAIN_WEBCLIENT_H_ */
