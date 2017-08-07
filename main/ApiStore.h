/*
 * ApiStore.h
 *
 *  Created on: 13.04.2017
 *      Author: helmut.spiegl
 */

#ifndef MAIN_APISTORE_H_
#define MAIN_APISTORE_H_

#include "nvs.h"
#include "String.h"
#include <list>

class ApiStore {
public:
	ApiStore();
	virtual ~ApiStore();

	void Init();

	bool SetApi(__uint8_t uId, const char* sApi);
	bool DeleteApi(__uint8_t uId);

	void GetApisJson(String& rsBody);

private:
	bool ReadApis();
	bool WriteApis();

private:
	std::list<String> mApis;

};

#endif /* MAIN_APISTORE_H_ */
