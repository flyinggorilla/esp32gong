/*
 * YuarelParser.hpp
 *
 *  Created on: 14.04.2017
 *      Author: bernd
 */

#ifndef MAIN_YuarelParser_HPP_
#define MAIN_YuarelParser_HPP_

#include "yuarel.h"
#include <string>
#include <string.h>

class UriParser {
public:
	UriParser();
	virtual ~UriParser();

	bool parseUrl(const char* url);

	/*
	 * @brief 	parses querystring into max 10 key/value pairs
	 * 			parser does not perform any string memory allocations, only pointers to existing strings
	 * @param	querystring
	 * @return	number of key/value pairs parsed
	 */
	int parseQuery(char* querystring);

	/*
	 * @brief	gets the URL-decoded query string value of a key
	 * 			Note: calling this twice on the same key could result in wrong results due to URL decoding of already decoded strings
	 * @return 	value string or NULL if key does not exist, or key does not have any value
	 *
	 */
	const char* getValueOf(const char* key);

	/*
	 * @brief	tests the existence of a key in the querystring
	 * @return 	true if key exists
	 *
	 */
	bool isKey(const char* key);


	const char* GetPath() { return msPath.c_str(); }
	const char* GetQuery() { return msQuery.c_str(); }
	const char* GetFragment() { return msFragment.c_str(); }
	const char* GetHost() { return msHost.c_str(); }
	bool GetSecure() { return mbSecure; }
	unsigned short GetPort() { return muPort; }


//	std::string UriDecode(const std::string & sSrc);
//
//	std::string UriEncode(const std::string & sSrc);

private:
	struct yuarel_param params[10];
	int queryValuepairs = 0;

	std::string msHost;
	bool mbSecure = false;
	std::string msFragment;
	std::string msQuery;
	std::string msPath;
	unsigned short muPort = 0;
};

#endif /* MAIN_YuarelParser_HPP_ */
