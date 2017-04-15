/*
 * UrlParser.cpp
 *
 *  Created on: 14.04.2017
 *      Author: bernd
 */

#include "UrlParser.hpp"
#include <string.h>
#include <string>
#include <stdio.h>
#include <esp_log.h>

UrlParser::UrlParser() {
	// TODO Auto-generated constructor stub

}

UrlParser::~UrlParser() {
	// TODO Auto-generated destructor stub
}

int UrlParser::parseQuery(char* querystring) {
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

const char* UrlParser::getValueOf(const char* key) {
	for (int i = 0; i < queryValuepairs; i++) {
		if (strcmp(key, params[i].key) == 0) {
			decode(params[i].val, params[i].val); // dont allocate memory, just decode inline
			return params[i].val;
		}
	}
	return NULL;
}



bool UrlParser::isKey(const char* key) {
	for (int i = 0; i < queryValuepairs; i++) {
		if (strcmp(key, params[i].key) == 0) {
			return true;
		}
	}
	return false;
}

// Uri encode and decode.
// RFC1630, RFC1738, RFC2396
// http://www.codeguru.com/cpp/cpp/algorithms/strings/article.php/c12759/URI-Encoding-and-Decoding.htm

//const char HEX2DEC[256] =
//{
//    /*       0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F */
//    /* 0 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
//    /* 1 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
//    /* 2 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
//    /* 3 */  0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1,
//
//    /* 4 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
//    /* 5 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
//    /* 6 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
//    /* 7 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
//
//    /* 8 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
//    /* 9 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
//    /* A */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
//    /* B */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
//
//    /* C */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
//    /* D */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
//    /* E */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
//    /* F */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1
//};

//std::string UrlParser::UriDecode(const std::string & sSrc)
//{
//    // Note from RFC1630:  "Sequences which start with a percent sign
//    // but are not followed by two hexadecimal characters (0-9, A-F) are reserved
//    // for future extension"
//
//    const unsigned char * pSrc = (const unsigned char *)sSrc.c_str();
//	const int SRC_LEN = sSrc.length();
//    const unsigned char * const SRC_END = pSrc + SRC_LEN;
//    const unsigned char * const SRC_LAST_DEC = SRC_END - 2;   // last decodable '%'
//
//    char * const pStart = new char[SRC_LEN];
//    char * pEnd = pStart;
//
//    while (pSrc < SRC_LAST_DEC)
//	{
//		if (*pSrc == '%')
//        {
//            char dec1, dec2;
//            if (-1 != (dec1 = HEX2DEC[*(pSrc + 1)])
//                && -1 != (dec2 = HEX2DEC[*(pSrc + 2)]))
//            {
//                *pEnd++ = (dec1 << 4) + dec2;
//                pSrc += 3;
//                continue;
//            }
//        }
//
//        *pEnd++ = *pSrc++;
//	}
//
//    // the last 2- chars
//    while (pSrc < SRC_END)
//        *pEnd++ = *pSrc++;
//
//    std::string sResult(pStart, pEnd);
//    delete [] pStart;
//	return sResult;
//}
//
//// Only alphanum is safe.
//const char SAFE[256] =
//{
//    /*      0 1 2 3  4 5 6 7  8 9 A B  C D E F */
//    /* 0 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
//    /* 1 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
//    /* 2 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
//    /* 3 */ 1,1,1,1, 1,1,1,1, 1,1,0,0, 0,0,0,0,
//
//    /* 4 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
//    /* 5 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,
//    /* 6 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
//    /* 7 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,
//
//    /* 8 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
//    /* 9 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
//    /* A */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
//    /* B */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
//
//    /* C */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
//    /* D */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
//    /* E */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
//    /* F */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
//};
//
//std::string UrlParser::UriEncode(const std::string & sSrc)
//{
//    const char DEC2HEX[16 + 1] = "0123456789ABCDEF";
//    const unsigned char * pSrc = (const unsigned char *)sSrc.c_str();
//    const int SRC_LEN = sSrc.length();
//    unsigned char * const pStart = new unsigned char[SRC_LEN * 3];
//    unsigned char * pEnd = pStart;
//    const unsigned char * const SRC_END = pSrc + SRC_LEN;
//
//    for (; pSrc < SRC_END; ++pSrc)
//	{
//		if (SAFE[*pSrc])
//            *pEnd++ = *pSrc;
//        else
//        {
//            // escape this char
//            *pEnd++ = '%';
//            *pEnd++ = DEC2HEX[*pSrc >> 4];
//            *pEnd++ = DEC2HEX[*pSrc & 0x0F];
//        }
//	}
//
//    std::string sResult((char *)pStart, (char *)pEnd);
//    delete [] pStart;
//    return sResult;
//}
