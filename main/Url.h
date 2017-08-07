#ifndef MAIN_URL_HPP_
#define MAIN_URL_HPP_

#include <stdio.h>
#include <list>

#include "String.h"

struct TQueryParam{
	String paramName;
	String paramValue;
};

class Url {
public:

	/*
	 * Url class to parse and build full http(s):// URLs or URL path segments.
	 **/
	Url();
	virtual ~Url();

	/* @brief construct a basic URL
	 * supply unencoded query string as part of path, or build build-up query string with AddQueryParam()
	 * use GetUrl() to retrieve then the full URL with properly URL encoded query strings
	 */
	void Build(bool bSecure, String& sHost, unsigned short uPort, String& sPath);
	void Build(bool bSecure, const char* host, unsigned short uPort, const char* path);

	/*
	 * @brief add unencoded querystring name/value pair
	 */
	void AddQueryParam(String& name, String& value);
	void AddQueryParam(const char* name, const char* value);

	/*
	 * @brief reconstructs a normalized http(s):// URL with URL encoded query string
	 */
	String& GetUrl();


	/* @brief parse URL and provide or manipulate its elements - note that optionally encoded query strings will be URL-decoded
	 * use GetUrl() to retrieve the rebuilt and URL encoded URL.
	 */
	bool Parse(String& sUrl);
	bool Parse(const char* url);


	/*
	 * @brief frees internal memory
	 */
	void Clear();

	bool GetSecure() { return mbSecure; }

	String& GetHost() { return msHost; }

	String& GetQuery();

	/* @return list of unencoded name/value pairs
	 */
	std::list<TQueryParam>& GetQueryParams() { return mlQueryParams; }

	String& GetPath() { return msPath; }

	String& GetFragment() { return msFragment; }


	void SetFragment(String& sFragment) { msFragment = sFragment; }
	void SetFragment(const char* fragment) { msFragment = fragment; }

	unsigned short GetPort() { return muPort; }
	String& GetPortAsString();

	bool Selftest();

	std::list<TQueryParam>& ParseQuery(String& u);

public:
	/*
	 * Helper methods to perform Url encoding  - can be used independently of the rest of the URL class
	 */
	String UrlEncode(String& str);

	/*
	 * Helper methods to perform Url encoding - can be used independently of the rest of the URL class
	 */
	String UrlDecode(String str);


private:
	std::list<String> mlRequestHeaders;

	unsigned short muPort = 0;
	bool mbSecure = false;
	String msUrl;
	String msHost;
	String msPath;
	String msFragment;
	String msQuery;
	std::list<TQueryParam> mlQueryParams;
	String msPort;

	bool ParseUrl(String& u);

};


#endif /* MAIN_URL_HPP_ */
