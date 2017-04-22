#ifndef MAIN_STRINGPARSER_H_
#define MAIN_STRINGPARSER_H_

#include "freertos/FreeRTOS.h"

#define MAX_STRINGS		5

class StringParser {
public:
	StringParser();
	virtual ~StringParser();

	void Init();
	bool AddStringToParse(const char* sParse);
	bool ConsumeChar(char c, bool ignoreLeadingSpaces = false);
	bool Found(__uint8_t& rIndex);

private:
	const char* mStringsToSearch[MAX_STRINGS];
	__uint8_t  mPos[MAX_STRINGS];
	__uint8_t  muCount;
	bool mbFirstChar;

};

#endif /* MAIN_STRINGPARSER_H_ */
