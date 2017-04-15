/*
 * UrlParser.hpp
 *
 *  Created on: 14.04.2017
 *      Author: bernd
 */

#ifndef MAIN_URLPARSER_HPP_
#define MAIN_URLPARSER_HPP_

#include "yuarel.h"

class UrlParser {
public:
	UrlParser();
	virtual ~UrlParser();

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

//	std::string UriDecode(const std::string & sSrc);
//
//	std::string UriEncode(const std::string & sSrc);

private:
	struct yuarel_param params[10];
	int queryValuepairs = 0;

};

#endif /* MAIN_URLPARSER_HPP_ */
