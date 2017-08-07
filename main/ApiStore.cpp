#include "ApiStore.h"
#include <freertos/FreeRTOS.h>
#include <esp_log.h>

#define MAX_APIS 20

ApiStore::ApiStore() {
}

ApiStore::~ApiStore() {
}

void ApiStore::Init(){

	if (!ReadApis()){
		mApis.push_back("/api?logo=ff0000|00ff00|0000ff|ff00cc");
		mApis.push_back("/api?logo_reset");
		mApis.push_back("/api?top_init&top=0|15|00ff00&top_morph=1000|5&bottom_init&bottom=0|15|00ff00&bottom_morph=1000|5");
		mApis.push_back("/api?top_init&top=0|15|ff0000&top_morph=80|8&bottom_init&bottom=0|15|ff0000&bottom_morph=80|8");
		mApis.push_back("/api?top_init&bottom_init");
		mApis.push_back("/api?top_init&top=0|1|ff0000&top_bg=00ff00&top_whirl=240|ccw&bottom_init&bottom=0|1|00ff00&bottom_bg=ff0000&bottom_whirl=190");
		mApis.push_back("/api?top_init&top=0|15|ff0000&top_morph=80|8&bottom_init&bottom=0|15|ff0000&bottom_morph=80|8&logo=ff0000|ff0000|ff0000|ff0000");
		mApis.push_back("/api?top_init&top=0|15|ff0000&bottom_init&bottom=0|15|00ff00");
	}
}

bool ApiStore::SetApi(__uint8_t uId, const char* sApi){
	ESP_LOGD("APISTORE", "SetApi(%d, %s)", uId, sApi);

	if (uId > MAX_APIS)
		return false;
	if (uId > mApis.size())
		return false;

	if (uId == mApis.size()){
		mApis.push_back(sApi);
	}
	else{
		__uint8_t u = 0;
		std::list<String>::iterator it = mApis.begin();
		while (it != mApis.end()){
			if (u == uId){
				(*it) = sApi;
				break;
			}
			u++;
			it++;
		}
	}
	return WriteApis();
}

bool ApiStore::DeleteApi(__uint8_t uId){
	ESP_LOGD("APISTORE", "DeleteApi(%d)", uId);

	if (uId >= mApis.size())
		return false;

	__uint8_t u = 0;
	std::list<String>::iterator it = mApis.begin();
	while (it != mApis.end()){
		if (u == uId){
			mApis.erase(it);
			break;
		}
		u++;
		it++;
	}
	return WriteApis();
}


void ApiStore::GetApisJson(String& rsBody){

	rsBody = "{\"apis\":[";
	std::list<String>::iterator it = mApis.begin();
	while (it != mApis.end()){

		rsBody += "\"";
		rsBody += (*it);
		rsBody += "\"";
		it++;
		if (it != mApis.end())
			rsBody += ",";
	}
	rsBody += "]}";
}

//------------------------------------------------------------------------------------------

bool ApiStore::ReadApis(){
	nvs_handle h;
	__uint32_t uLen = 0;
	esp_err_t ret;

	if (nvs_open("Api", NVS_READONLY, &h) != ESP_OK)
		return false;

	ret = nvs_get_blob(h, "Apis", NULL, &uLen);
	if ((ret != ESP_OK) || !uLen){
		ESP_LOGD("APISTORE", "nvs_get_blob (first) returned ERROR %x", ret);
		nvs_close(h);
		return false;
	}
	char* sBuf = (char*)malloc(uLen + 1);
	if (!sBuf)
		return nvs_close(h), false;
	ret = nvs_get_blob(h, "Apis", sBuf, &uLen);
	if (ret != ESP_OK){
		ESP_LOGD("APISTORE", "nvs_get_blob returned ERROR %x", ret);
		free(sBuf);
		nvs_close(h);
		return false;
	}
	sBuf[uLen] = 0x00;
	mApis.clear();

	mApis.push_back(sBuf);
	bool bZeroFound = false;
	for (__uint32_t u=0 ; u<uLen ; u++){
		if (bZeroFound){
			mApis.push_back(sBuf + u);
			bZeroFound = false;
		}
		else{
			if (!sBuf[u])
				bZeroFound = true;
		}
	}
	free (sBuf);
	nvs_close(h);
	return true;
}

bool ApiStore::WriteApis(){
	nvs_handle h;
	esp_err_t ret;

	if (nvs_open("Api", NVS_READWRITE, &h) != ESP_OK)
		return false;

	if (!mApis.size()){
		ret = nvs_erase_all(h);
		nvs_close(h);
		return ret == ESP_OK;
	}

	__uint16_t uSize = 0;
	std::list<String>::iterator it = mApis.begin();
	while (it != mApis.end()){
		uSize += (*it).length() + 1;
		it++;
	}
	char* sBuf = (char*)malloc(uSize);
	if (!sBuf)
		return nvs_close(h), false;
	__uint16_t uPos = 0;
	it = mApis.begin();
	while (it != mApis.end()){
		strcpy(sBuf + uPos, (*it).c_str());
		uPos += (*it).length();
		sBuf[uPos++] = 0x00;
		it++;
	}
	ret = nvs_set_blob(h, "Apis", sBuf, uSize);
	free(sBuf);
	nvs_close(h);
	return ret == ESP_OK;
}



