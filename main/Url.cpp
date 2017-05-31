#include "Url.h"
#include <esp_log.h>
#include <sstream>
#include <iomanip>
#include <stdlib.h>
#include <stdio.h>
#include <list>

static const char LOGTAG[] = "Url";

Url::Url() {
	Clear();
}

void Url::Build(bool bSecure, String& sHost, unsigned short uPort, String& sPath) {
	msHost = sHost;
	msPath = sPath;
	mbSecure = bSecure;
	muPort = uPort;
}

void Url::Build(bool bSecure, const char* host, unsigned short uPort, const char* path) {
	msHost = host;
	msPath = path;
	mbSecure = bSecure;
	muPort = uPort;
}

bool Url::Parse(String& sUrl) {
	Clear();
	if (!ParseUrl(sUrl)) {
		ESP_LOGI(LOGTAG, "Invalid URL: '%s' (parsed: '%s')", sUrl.c_str(), GetUrl().c_str());
		return false;
	}
	return true;
}

bool Url::Parse(const char* url) {
	String sUrl = url;
	return Parse(sUrl);
}

Url::~Url() {

}

void Url::Clear() {
	muPort = 0;
	mbSecure = false;
	msUrl.clear();
	msHost.clear();
	msPath.clear();
	msFragment.clear();
	msQuery.clear();
	mlQueryParams.clear();
}

String& Url::GetPortAsString() {
	//char buf[20];
	//sprintf(buf, "%hu", muPort);
	//msPort = buf;
	msPort = muPort;
	return msPort;
}

String& Url::GetUrl() {
	msUrl.clear();
	if (msHost.length()) {
		msUrl += mbSecure ? "https://" : "http://";
		msUrl += msHost;

		if (!(muPort == 80 && !mbSecure) && !(muPort == 443 && mbSecure)) {
			msUrl += ':';
			//char buf[20];
			//sprintf(buf, "%hu", muPort);
			//msUrl += buf;
			msUrl += muPort;
		}
	}

	if (!msPath.length() || msPath[0] != '/') {
		msUrl += '/';
	}
	msUrl += msPath;

	if (mlQueryParams.size()) {
		msUrl += '?';
		msUrl += GetQuery();
	}
	if (msFragment.length()) {
		msUrl += '#';
		msUrl += msFragment;

	}
	return msUrl;

}

void Url::AddQueryParam(String& name, String& value) {
	TQueryParam param;
	param.paramName = name;
	param.paramValue = value;
	mlQueryParams.push_back(param);
}

void Url::AddQueryParam(const char* name, const char* value) {
	TQueryParam param;
	param.paramName = name ? name : "";
	param.paramValue = value ? value : "";
	mlQueryParams.push_back(param);
}

String& Url::GetQuery() {
	msQuery.clear();
	for (std::list<TQueryParam>::iterator it = mlQueryParams.begin(); it != mlQueryParams.end(); ++it) {
		msQuery += UrlEncode((*it).paramName);
		msQuery += "=";
		msQuery += UrlEncode((*it).paramValue);
		if (it != mlQueryParams.end()) {
			msQuery += "&";
		}
	}
	return msQuery;
}

String Url::UrlEncode(String& str) {
	std::ostringstream escaped;
	escaped.fill('0');
	escaped << std::hex;

	for (unsigned int i = 0, n = str.length(); i != n; ++i) {
		char c = str[i];

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

	return String(escaped.str().c_str());
}

String Url::UrlDecode(String str) {
	String ret;
	char ch;
	int i, ii, len = str.length();

	for (i = 0; i < len; i++) {
		if (str[i] != '%') {
			if (str[i] == '+')
				ret += ' ';
			else
				ret += str[i];
		} else {
			sscanf(str.substring(i + 1, 2).c_str(), "%x", &ii);
			ch = static_cast<char>(ii);
			ret += ch;
			i = i + 2;
		}
	}
	return ret;
}

bool Url::Selftest() {
	String s = "http://www.xyz.com?name1=param1&name2&name3=&name4=val4&n a m e 5=v%a%l%u%e5#fragment1";
	Url u;
	u.Parse(s);
	ESP_LOGI(LOGTAG, "Test input  URL: %s", s.c_str());
	ESP_LOGI(LOGTAG, "Test output URL: %s", u.GetUrl().c_str());
	ESP_LOGI(LOGTAG, "Test host<%s:%hu> path<%s> query<%s> fragment<%s>", u.GetHost().c_str(), u.GetPort(),
			u.GetPath().c_str(), u.GetQuery().c_str(), u.GetFragment().c_str());

	s = u.GetUrl();
	u.Parse(s);
	ESP_LOGI(LOGTAG, "Test input  URL: %s", s.c_str());
	ESP_LOGI(LOGTAG, "Test output URL: %s", u.GetUrl().c_str());
	ESP_LOGI(LOGTAG, "Test host<%s:%hu> path<%s> query<%s> fragment<%s>", u.GetHost().c_str(), u.GetPort(),
			u.GetPath().c_str(), u.GetQuery().c_str(), u.GetFragment().c_str());

	const char* cs = "https://xyz.com:765?name1=param1&name2&name3=&name4=val4&n a m e 5=v%a%l%u%e5#fragment1";
	u.Parse(cs);
	ESP_LOGI(LOGTAG, "Test input  URL: %s", cs);
	ESP_LOGI(LOGTAG, "Test output URL: %s", u.GetUrl().c_str());
	ESP_LOGI(LOGTAG, "Test host<%s:%hu> path<%s> query<%s> fragment<%s>", u.GetHost().c_str(), u.GetPort(),
			u.GetPath().c_str(), u.GetQuery().c_str(), u.GetFragment().c_str());

	s = "/relativePath?#falsefragment&#=#&?=?=2==&1&&#fragment1";
	u.Parse(s);
	ESP_LOGI(LOGTAG, "Test input  URL: %s", s.c_str());
	ESP_LOGI(LOGTAG, "Test output URL: %s", u.GetUrl().c_str());
	ESP_LOGI(LOGTAG, "Test host<%s:%hu> path<%s> query<%s> fragment<%s>", u.GetHost().c_str(), u.GetPort(),
			u.GetPath().c_str(), u.GetQuery().c_str(), u.GetFragment().c_str());

	s = "/relativePath?#falsefragment&#=#&?=?=2==&1&&#fragment1";
	u.Clear();
	u.Build(false, "testhost", 0, "mypath");
	u.AddQueryParam("#falsefragment", NULL);
	u.AddQueryParam("#", "#");
	u.AddQueryParam("?=?=2==", "");
	u.AddQueryParam("1", "");
	u.AddQueryParam("", "");
	u.SetFragment("fragment1");
	ESP_LOGI(LOGTAG, "Test input  URL: %s", s.c_str());
	ESP_LOGI(LOGTAG, "Test output URL: %s", u.GetUrl().c_str());
	ESP_LOGI(LOGTAG, "Test host<%s:%hu> path<%s> query<%s> fragment<%s>", u.GetHost().c_str(), u.GetPort(),
			u.GetPath().c_str(), u.GetQuery().c_str(), u.GetFragment().c_str());

	return true;
}

#define STATE_RelativeUrl 0
#define STATE_Http 1
#define STATE_Host 2
#define STATE_Port 3
#define STATE_Path 4
#define STATE_Query 5
#define STATE_Fragment 6

bool Match(unsigned int& it, String& s, String match) {
	unsigned int mit = 0;
	while (mit != match.length()) {
		if (it == s.length()) {
			return false;
		}
		if (tolower(s[it]) != tolower(match[mit])) {
			return false;
		}
		mit++;
		it++;
	}
	return true;
}

bool Url::ParseUrl(String& u) {

	String scheme;

	char c;
	unsigned short state = STATE_RelativeUrl;

	unsigned int it = 0;
	while (it != u.length()) {
		c = u[it];

		switch (state) {
		case STATE_RelativeUrl:
			if (c == '/') {
				state = STATE_Path;
				msPath += '/';
				break;
			}
			state = STATE_Http;
			// dont break here;

		case STATE_Http:
			if (c == ' ') { // skip leading spaces
				break;
			}

			if (!Match(it, u, "http")) {
				return false;
			}

			if (Match(it, u, "s")) {
				mbSecure = true;
			}

			if (!Match(it, u, "://")) {
				return false;
			}
			c = u[it];
			state = STATE_Host;
			// dont break here

		case STATE_Host:
			if (c == ':') {
				state = STATE_Port;
				break;
			}
			if (c == '?') {
				state = STATE_Query;
				break;
			}
			if (c == '#') {
				state = STATE_Fragment;
				break;
			}
			if (c == '/') {
				state = STATE_Path;
				msPath += '/';
				break;
			}
			msHost += c;
			break;

		case STATE_Port:
			if (c == '?') {
				state = STATE_Query;
				break;
			}
			if (c == '#') {
				state = STATE_Fragment;
				break;
			}
			if (c == '/') {
				state = STATE_Path;
				msPath += '/';
				break;

			}
			if ((c >= '0') && (c <= '9')) {
				muPort *= 10;
				muPort += c - '0';
				break;
			}
			break;

		case STATE_Path:
			if (c == '?') {
				state = STATE_Query;
				break;
			}
			if (c == '#') {
				state = STATE_Fragment;
				break;
			}
			msPath += c;
			break;

		case STATE_Query:
			if (c == '#') {
				state = STATE_Fragment;
				break;
			}
			msQuery += c;
			break;

		case STATE_Fragment:
			msFragment += c;
			break;
		}

		it++;
	}
	if (!msPath.length()) {
		msPath += '/';
	}

	if (!muPort) {
		muPort = mbSecure ? 443 : 80;
	}

	ParseQuery(msQuery);
	return true;
}

#define STATE_QueryName 0
#define STATE_QueryValue 1

std::list<TQueryParam>& Url::ParseQuery(String& u) {
	mlQueryParams.clear();
	if (u.length() == 0) {
		return mlQueryParams;
	}

	String name;
	String value;

	unsigned int it;
	it = 0;

	char c;
	unsigned short state = STATE_QueryName;

	while (it != u.length()) {
		c = u[it];

		switch (state) {
		case STATE_QueryName:
			if (c == '=') {
				state = STATE_QueryValue;
				break;
			}
			if (c == '&') {
				mlQueryParams.emplace_back();
				mlQueryParams.back().paramName = UrlDecode(name);
				mlQueryParams.back().paramValue = UrlDecode(value);
				name.clear();
				value.clear();
				break;
			}
			name += c;
			break;

		case STATE_QueryValue:
			if (c == '&') {
				state = STATE_QueryName;
				mlQueryParams.emplace_back();
				mlQueryParams.back().paramName = UrlDecode(name);
				mlQueryParams.back().paramValue = UrlDecode(value);
				name.clear();
				value.clear();
				break;
			}
			value += c;
			break;

		}
		it++;
	}
	mlQueryParams.emplace_back();
	mlQueryParams.back().paramName = UrlDecode(name);
	mlQueryParams.back().paramValue = UrlDecode(value);
	return mlQueryParams;
}

