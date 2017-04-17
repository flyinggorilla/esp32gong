/*
 * YuarelParser.cpp
 *
 *  Created on: 14.04.2017
 *      Author: bernd
 */

#include "UriParser.hpp"
#include <string.h>
#include <string>
#include <stdio.h>
#include <esp_log.h>

static const char LOGTAG[] = "UriParser";

UriParser::UriParser() {
	// TODO Auto-generated constructor stub

}

UriParser::~UriParser() {
	// TODO Auto-generated destructor stub
}

int UriParser::parseQuery(char* querystring) {
	return queryValuepairs = yuarel_parse_query(querystring, '&', params, sizeof(params));
}


inline int ishex(int x)
{
	return	(x >= '0' && x <= '9')	||
		(x >= 'a' && x <= 'f')	||
		(x >= 'A' && x <= 'F');
}

int decode(const char *s, char *dec)
{
	char *o;
	const char *end = s + strlen(s);
	int c;

	for (o = dec; s <= end; o++) {
		c = *s++;
		if (c == '+') c = ' ';
		else if (c == '%' && (	!ishex(*s++)	||
					!ishex(*s++)	||
					!sscanf(s - 2, "%2x", &c)))
			return -1;

		if (dec) *o = c;
	}

	return o - dec;
}

const char* UriParser::getValueOf(const char* key) {
	for (int i = 0; i < queryValuepairs; i++) {
		if (strcmp(key, params[i].key) == 0) {
			decode(params[i].val, params[i].val); // dont allocate memory, just decode inline
			return params[i].val;
		}
	}
	return NULL;
}



bool UriParser::isKey(const char* key) {
	for (int i = 0; i < queryValuepairs; i++) {
		if (strcmp(key, params[i].key) == 0) {
			return true;
		}
	}
	return false;
}

bool UriParser::parseUrl(const char* url) {
	struct yuarel uri;

	std::string u(url);
	if (yuarel_parse(&uri, (char*)u.data() )) {
		return false; //error
	}

	ESP_LOGI(LOGTAG, "step1");

	if (!uri.host) {
		return false;
	}
	msHost = uri.host;

	mbSecure = false;
	if (!strcmp(uri.scheme, "https")) {
		mbSecure = true;
	} else if (strcmp(uri.scheme, "http")) {
		return false;
	}

	ESP_LOGI(LOGTAG, "step2");

	if (uri.fragment) {
		msFragment = uri.fragment;
	}

	if (uri.query) {
		msQuery = uri.query;
	}

	if (uri.path) {
		msPath = uri.path;
	}

	if (uri.port) {
	   muPort = uri.port;
	} else {
		muPort = mbSecure ? 443 : 80;
	}
	return true;
}
