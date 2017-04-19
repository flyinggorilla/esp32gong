#include "Url.hpp"
#include <esp_log.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdlib.h>
#include <stdio.h>
#include <list>
#include "UriParser.hpp"

static const char LOGTAG[] = "Url";

Url::Url(std::string& sHost, short int iPort, bool bSecure, std::string& sPath) {
	msHost = sHost;
	msPath = sPath;
	mbSecure = bSecure;
	miPort = iPort;
}

Url::Url(std::string& sUrl) {
	ESP_LOGI(LOGTAG, "HttpPrepareGet: %s", sUrl.c_str());
	if (!ParseUrl(sUrl.c_str())) {
		ESP_LOGE(LOGTAG, "Invalid URI %s", sUrl.c_str());
		return;
	}

	ESP_LOGI(LOGTAG, "after url parsing: hostname=<%s> path=<%s>", msHost.c_str(), msPath.c_str());
}

Url::~Url() {

}

std::string& Url::GetUrl() {
	msUrl.clear();
	if (!msHost.empty()) {
		msUrl += mbSecure ? "https://" : "http://";
		msUrl += msHost;
		if (miPort) {
			msUrl += ':';
			char buf[20];
			sprintf(buf, "%i", miPort);
			msUrl += buf;
		}
	}
	msUrl += '/'; //TODO should this be added?
	msUrl += msPath;
	if (mlQueryParams.size()) {
		msUrl += '?';
		msUrl += GetQuery();
	}
	if (!msFragment.empty()) {
		msUrl += '#';
		msUrl += msFragment;

	}
	return msUrl;

}

void Url::AddQueryParam(std::string& name, std::string& value) {
	TParam param;
	param.paramName = name;
	param.paramValue = value;
	mlQueryParams.push_back(param);
}

std::string& Url::GetQuery() {
	msQuery.clear();
	for (std::list<TParam>::iterator it = mlQueryParams.begin(); it != mlQueryParams.end(); ++it) {
		msQuery += UrlEncode((*it).paramName);
		msQuery += "=";
		msQuery += UrlEncode((*it).paramValue);
		if (it != mlQueryParams.end()) {
			msQuery += "&";
		}
	}
	return msQuery;
}

std::string Url::UrlEncode(std::string& str) {
	/*std::string new_str = "";
	 char c;
	 int ic;
	 const char* chars = str.c_str();
	 char bufHex[10];
	 int len = strlen(chars);

	 for(int i=0;i<len;i++){
	 c = chars[i];
	 ic = c;
	 // uncomment this if you want to encode spaces with +
	 //if (c==' ') new_str += '+';
	 //else
	 if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') new_str += c;
	 else {
	 sprintf(bufHex,"%X",c);
	 if(ic < 16)
	 new_str += "%0";
	 else
	 new_str += "%";
	 new_str += bufHex;
	 }
	 }
	 return new_str;*/
	std::ostringstream escaped;
	escaped.fill('0');
	escaped << std::hex;

	for (std::string::const_iterator i = str.begin(), n = str.end(); i != n; ++i) {
		std::string::value_type c = (*i);

		// Keep alphanumeric and other accepted characters intact
		if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
			escaped << c;
			continue;
		}

		// Any other characters are percent-encoded
		escaped << std::uppercase;
		escaped << '%' << std::setw(2) << int((unsigned char) c);
		escaped << std::nouppercase;
	}

	return escaped.str();
}

std::string Url::UrlDecode(std::string str) {
	std::string ret;
	char ch;
	int i, ii, len = str.length();

	for (i = 0; i < len; i++) {
		if (str[i] != '%') {
			if (str[i] == '+')
				ret += ' ';
			else
				ret += str[i];
		} else {
			sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
			ch = static_cast<char>(ii);
			ret += ch;
			i = i + 2;
		}
	}
	return ret;
}

//TODO replace with own parser
int Url::ParseQuery(char* querystring) {
	struct yuarel_param params[10];
	int queryValuepairs = yuarel_parse_query(querystring, '&', params, sizeof(params));
	for (int i = 0; i < queryValuepairs; i++) {
		TParam param { params[i].key ? params[i].key : "", params[i].val ? params[i].val : "" };
		mlQueryParams.emplace_back(param);
	}
	return queryValuepairs;
}

inline int ishex(int x) {
	return (x >= '0' && x <= '9') || (x >= 'a' && x <= 'f') || (x >= 'A' && x <= 'F');
}

int decode(const char *s, char *dec) {
	char *o;
	const char *end = s + strlen(s);
	int c;

	for (o = dec; s <= end; o++) {
		c = *s++;
		if (c == '+')
			c = ' ';
		else if (c == '%' && (!ishex(*s++) || !ishex(*s++) || !sscanf(s - 2, "%2x", &c)))
			return -1;

		if (dec)
			*o = c;
	}

	return o - dec;
}

/*
 const char* Url::GetValueOf(const char* key) {
 for (int i = 0; i < queryValuepairs; i++) {
 if (strcmp(key, params[i].key) == 0) {
 decode(params[i].val, params[i].val); // dont allocate memory, just decode inline
 return params[i].val;
 }
 }
 return NULL;
 }



 bool Url::isKey(const char* key) {
 for (int i = 0; i < queryValuepairs; i++) {
 if (strcmp(key, params[i].key) == 0) {
 return true;
 }
 }
 return false;
 }*/

bool Url::ParseUrl(std::string url) {
	struct yuarel uri;

	if (yuarel_parse(&uri, (char*) url.data())) {
		return false; //error
	}

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

	if (uri.fragment) {
		msFragment = uri.fragment;
	}

	if (uri.query) {
		msQuery = uri.query;
		int i = ParseQuery(uri.query);
		ESP_LOGI(LOGTAG, "ParseQuery(%i): %s", i, uri.query);
	}

	if (uri.path) {
		msPath = uri.path;
	}

	if (uri.port) {
		miPort = uri.port;
	} else {
		miPort = mbSecure ? 443 : 80;
	}
	return true;
}

bool Url::Selftest() {
	std::string s = "https://xyz.com:765?name1=param1&name2&name3=&name4=val4&n a m e 5=v%a%l%u%e5#fragment1";
	//TODO handle path "/" ---> after url parsing: hostname=<xyz.com> path=<>

//	I (4764) Url: HttpPrepareGet: https://xyz.com:765?name1=param1&name2&name3=&name4=val4&n a m e 5&v%a%l%u%e5#fragment1
//	I (4774) Url: step1
//	I (4774) Url: step2
//	I (4774) Url: after url parsing: hostname=<xyz.com> path=<>
//	I (4784) Url: Test input URL: https://xyz.com:765?name1=param1&name2&name3=&name4=val4&n a m e 5&v%a%l%u%e5#fragment1
//	I (4794) Url: Test output URL: https://xyz.com:/#fragment1
//	I (4804) Url: Test Url(): xyz.com:765/?#fragment1


	Url u(s);
	ESP_LOGI(LOGTAG, "Test input URL: %s", s.c_str());
	ESP_LOGI(LOGTAG, "Test output URL: %s", u.GetUrl().c_str());
	ESP_LOGI(LOGTAG, "Test Url(): %s:%i/%s?%s#%s", u.GetHost().c_str(), (int )u.GetPort(), u.GetPath().c_str(),
			u.GetQuery().c_str(), u.GetFragment().c_str());



	return true;
}
