#include <freertos/FreeRTOS.h>
#include "Config.h"
#include "nvs_flash.h"
#include <esp_log.h>


#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PASS CONFIG_WIFI_PASSWORD


Config::Config() {
	mbAPMode = true;
	msAPSsid = "esp32gong";
	msHostname = "esp32gong";

	msSTASsid = WIFI_SSID;
	msSTAPass = WIFI_PASS;

	mbWebServerUseSsl = false;
	muWebServerPort = 0;

	muLastSTAIpAddress = 0;
}

Config::~Config() {
}

bool Config::Read(){
	nvs_handle h;

	if (nvs_flash_init() != ESP_OK)
		return false;
	if (nvs_open("Ufo Config", NVS_READONLY, &h) != ESP_OK)
		return false;
	ReadBool(h, "APMode", mbAPMode);
	ReadString(h, "APSsid", msAPSsid);
	ReadString(h, "APPass", msAPPass);
	nvs_get_u32(h, "STAIpAddress", &muLastSTAIpAddress);
	ReadString(h, "STASsid", msSTASsid);
	ReadString(h, "STAPass", msSTAPass);
	ReadString(h, "STAENTUser", msSTAENTUser);
	ReadString(h, "STAENTCA", msSTAENTCA);
	ReadString(h, "hostname", msHostname);

	nvs_close(h);
	return true;
}

bool Config::Write()
{
	nvs_handle h;

	if (nvs_flash_init() != ESP_OK)
		return false;
	if (nvs_open("Ufo Config", NVS_READWRITE, &h) != ESP_OK)
		return false;
	nvs_erase_all(h); //otherwise I need double the space

	if (!WriteBool(h, "APMode", mbAPMode))
		return nvs_close(h), false;
	if (!WriteString(h, "APSsid", msAPSsid))
		return nvs_close(h), false;
	if (!WriteString(h, "APPass", msAPPass))
		return nvs_close(h), false;
	if (!WriteString(h, "STASsid", msSTASsid))
		return nvs_close(h), false;
	if (!WriteString(h, "STAPass", msSTAPass))
		return nvs_close(h), false;
	if (!WriteString(h, "hostname", msHostname))
		return nvs_close(h), false;
	if (!WriteString(h, "STAENTUser", msSTAENTUser))
		return nvs_close(h), false;
	if (!WriteString(h, "STAENTCA", msSTAENTCA))
		return nvs_close(h), false;
	if (nvs_set_u32(h, "STAIpAddress", muLastSTAIpAddress) != ESP_OK)
		return nvs_close(h), false;

	nvs_commit(h);
	nvs_close(h);
	return true;
}

//------------------------------------------------------------------------------------


bool Config::ReadString(nvs_handle h, const char* sKey, std::string& rsValue){
	char* sBuf = NULL;
	__uint32_t u = 0;

	nvs_get_str(h, sKey, NULL, &u);
	if (!u)
		return false;
	sBuf = (char*)malloc(u+1);
	if (nvs_get_str(h, sKey, sBuf, &u) != ESP_OK)
		return free(sBuf), false;
	sBuf[u] = 0x00;
	rsValue = sBuf;
	free(sBuf);
	return true;
}

bool Config::ReadBool(nvs_handle h, const char* sKey, bool& rbValue){
	__uint8_t u;
	if (nvs_get_u8(h, sKey, &u) != ESP_OK)
		return false;
	rbValue = u ? true : false;
	return true;
}

bool Config:: WriteString(nvs_handle h, const char* sKey, std::string& rsValue){
	esp_err_t err = nvs_set_str(h, sKey, rsValue.data());
	if (err != ESP_OK){
		ESP_LOGD("CONFIG", "!!!!!!!!!!!!!!!!!!! Error %d", err);
		return false;
	}
	return true;
}


bool Config:: WriteBool(nvs_handle h, const char* sKey, bool bValue){
	return (nvs_set_u8(h, sKey, bValue ? 1 : 0) == ESP_OK);
}
