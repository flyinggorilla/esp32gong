#include "StringParser.h"
#include <_ansi.h>
#include <ctype.h>


StringParser::StringParser() {
	Init();
}

StringParser::~StringParser() {

}

void StringParser::Init(){
	muCount = 0;
	mbFirstChar = true;
}

bool StringParser::AddStringToParse(const char* sParse){
	if (muCount >= MAX_STRINGS)
		return false;

	mStringsToSearch[muCount] = sParse;
	mPos[muCount] = 0;
	muCount++;
	return true;
}

bool StringParser::ConsumeChar(char c, bool ignoreLeadingSpaces){
	if (!c)
		return false;
	c = tolower(c);

	bool oneActive = false;
	if (mbFirstChar){
		if (!ignoreLeadingSpaces || (c != ' ')){
			for (__uint8_t u=0 ; u<muCount ; u++){
				if (mStringsToSearch[u][0] == c){
					mPos[u] = 1;
					oneActive = true;
				}
			}
			mbFirstChar = false;
		}
		return true;
	}
	else{
		for (__uint8_t u=0 ; u<muCount ; u++){
			if (mPos[u]){
				if (mStringsToSearch[u][mPos[u]] == c){
					mPos[u]++;
					oneActive = true;
				}
				else
					mPos[u] = 0;
			}
		}

	}
	return oneActive;
}
void StringParser::ConsumeCharSimple(char c){
	for (__uint8_t u=0 ; u<muCount ; u++){
		if (mStringsToSearch[u][mPos[u]] == c)
			mPos[u]++;
		else{
			if (mStringsToSearch[u][0] == c)
				mPos[u] = 1;
			else
				mPos[u] = 0;
		}
	}	
}


bool StringParser::Found(__uint8_t& rIndex){
	for (__uint8_t u=0 ; u<muCount ; u++){
		if (mPos[u] && mStringsToSearch[u][mPos[u]] == 0x00){
			rIndex = u;
			return true;
		}
	}
	return false;
}

