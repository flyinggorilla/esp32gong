#ifndef MAIN_URL_HPP_
#define MAIN_URL_HPP_

#include <stdio.h>
#include <list>
#include <string>

struct TQueryParam{
	std::string paramName;
	std::string paramValue;
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
	void Build(bool bSecure, std::string& sHost, unsigned short uPort, std::string& sPath);
	void Build(bool bSecure, const char* host, unsigned short uPort, const char* path);

	/*
	 * @brief add unencoded querystring name/value pair
	 */
	void AddQueryParam(std::string& name, std::string& value);
	void AddQueryParam(const char* name, const char* value);

	/*
	 * @brief reconstructs a normalized http(s):// URL with URL encoded query string
	 */
	std::string& GetUrl();


	/* @brief parse URL and provide or manipulate its elements - note that optionally encoded query strings will be URL-decoded
	 * use GetUrl() to retrieve the rebuilt and URL encoded URL.
	 */
	bool Parse(std::string& sUrl);
	bool Parse(const char* url);


	/*
	 * @brief frees internal memory
	 */
	void Clear();

	bool GetSecure() { return mbSecure; }

	std::string& GetHost() { return msHost; }

	std::string& GetQuery();

	/* @return list of unencoded name/value pairs
	 */
	std::list<TQueryParam>& GetQueryParams() { return mlQueryParams; }

	std::string& GetPath() { return msPath; }

	std::string& GetFragment() { return msFragment; }


	void SetFragment(std::string& sFragment) { msFragment = sFragment; }
	void SetFragment(const char* fragment) { msFragment = fragment; }

	unsigned short GetPort() { return muPort; }
	std::string& GetPortAsString();

	bool Selftest();

	std::list<TQueryParam>& ParseQuery(std::string& u);

public:
	/*
	 * Helper methods to perform Url encoding  - can be used independently of the rest of the URL class
	 */
	std::string UrlEncode(std::string& str);

	/*
	 * Helper methods to perform Url encoding - can be used independently of the rest of the URL class
	 */
	std::string UrlDecode(std::string str);


private:
	std::list<std::string> mlRequestHeaders;

	unsigned short muPort = 0;
	bool mbSecure = false;
	std::string msUrl;
	std::string msHost;
	std::string msPath;
	std::string msFragment;
	std::string msQuery;
	std::list<TQueryParam> mlQueryParams;
	std::string msPort;

	bool ParseUrl(std::string& u);

};


#endif /* MAIN_URL_HPP_ */
