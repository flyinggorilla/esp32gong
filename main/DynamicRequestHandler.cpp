#include "DynamicRequestHandler.h"
#include "Config.h"
#include "esp_system.h"
#include <esp_log.h>

#include "esp_system.h"
#include <esp_log.h>
#include <cJSON.h>
#include "Config.h"
#include "Esp32Gong.h"
#include "I2SPlayer.h"
#include "Wifi.h"
#include "Ota.h"

extern Wifi wifi;
extern Esp32Gong esp32gong;
extern I2SPlayer musicPlayer;

static const char LOGTAG[] = "WebSrvDRH";

DynamicRequestHandler::DynamicRequestHandler() {
	mbRestart = false;
}

DynamicRequestHandler::~DynamicRequestHandler() {

}

bool DynamicRequestHandler::HandleApiRequest(std::list<TParam>& params, HttpResponse& rResponse) {

	std::string sBody;
	std::list<TParam>::iterator it = params.begin();
	while (it != params.end()) {

		if ((*it).paramName == "gong") {
			musicPlayer.setVolume(25);
			musicPlayer.playAsync();
			sBody = "<html><body>api call - lets play music</html></body>";
		}

		it++;
	}

	rResponse.AddHeader(HttpResponse::HeaderNoCache);
	rResponse.SetRetCode(200);
	return rResponse.Send(sBody.data(), sBody.size());
}


bool DynamicRequestHandler::HandleApiListRequest(std::list<TParam>& params, HttpResponse& rResponse) {
	std::string sBody;
	//esp32gong.GetApiStore().GetApisJson(sBody);
	rResponse.AddHeader(HttpResponse::HeaderNoCache);
	rResponse.SetRetCode(200);
	return rResponse.Send(sBody.data(), sBody.size());
}
bool DynamicRequestHandler::HandleApiEditRequest(std::list<TParam>& params, HttpResponse& rResponse) {

	/*__uint8_t uId = 0xff;
	const char* sNewApi = NULL;
	bool bDelete = false;

	std::list<TParam>::iterator it = params.begin();
	while (it != params.end()) {

		if ((*it).paramName == "apiid")
			uId = strtol((*it).paramValue.data(), NULL, 10) - 1;
		else if ((*it).paramName == "apiedit")
			sNewApi = (*it).paramValue.data();
		else if ((*it).paramName == "delete")
			bDelete = true;
		it++;
	}
	if (bDelete) {
		if (!esp32gong.GetApiStore().DeleteApi(uId))
			rResponse.SetRetCode(500);
	} else {
		if (!esp32gong.GetApiStore().SetApi(uId, sNewApi))
			rResponse.SetRetCode(500);
	}
	rResponse.AddHeader("Location: /"); */
	rResponse.AddHeader(HttpResponse::HeaderNoCache);
	rResponse.SetRetCode(302);
	return rResponse.Send();
}

bool DynamicRequestHandler::HandleFirmwareRequest(std::list<TParam>& params, HttpResponse& response) {
	std::list<TParam>::iterator it = params.begin();
	std::string sBody;
	response.SetRetCode(400); // invalid request
	while (it != params.end()) {
		if ((*it).paramName == "update") {
			//Ota::StartUpdateFirmwareTask();
			ESP_LOGW(LOGTAG, "Starting Firmware Update Task ....");

			Ota ota;
			if(ota.UpdateFirmware("http://surpro4:9999/getfirmware")) {
				ESP_LOGI(LOGTAG, "AFTER OTA STUFF---- RESTARTING IN 2 SEC");
				mbRestart = true;
				sBody = "Firmware update process initiated......";
				response.SetRetCode(200);
			} else {
				//TODO add ota.GetErrorInfo() to inform end-user of problem
				ESP_LOGE(LOGTAG, "OTA update failed!");
				sBody = "Firmware update failed. Rebooting anyway.";
				response.SetRetCode(500);
				mbRestart = true;
			}
		} else if ((*it).paramName == "check") {
			//TODO implement firmware version check;
			sBody = "not implemented";
			response.SetRetCode(501); // not implemented
		}
		it++;
	}
	response.AddHeader(HttpResponse::HeaderNoCache);
	response.AddHeader(HttpResponse::HeaderContentTypeJson);
	return response.Send(sBody.data(), sBody.size());
}

bool DynamicRequestHandler::HandleInfoRequest(std::list<TParam>& params, HttpResponse& rResponse) {

	std::string sBody;

	/*	char sBuf[100];
	 char sHelp[20];

	 sBody = "{";
	 sprintf(sBuf, "\"apmode\":\"%d\",", esp32gong.GetConfig().mbAPMode);
	 sBody += sBuf;
	 sprintf(sBuf, "\"heap\":\"%d Bytes\",", esp_get_free_heap_size());
	 sBody += sBuf;
	 sprintf(sBuf, "\"ssid\":\"%s\",", esp32gong.GetConfig().msSTASsid.data());
	 sBody += sBuf;

	 if (esp32gong.GetConfig().mbAPMode){
	 sprintf(sBuf, "\"lastiptoap\":\"%d.%d.%d.%d\",", IP2STR((ip4_addr*)&(esp32gong.GetConfig().muLastSTAIpAddress)));
	 sBody += sBuf;
	 }
	 else{
	 esp32gong.GetWifi().GetLocalAddress(sHelp);
	 sprintf(sBuf, "\"ipaddress\":\"%s\",", sHelp);
	 sBody += sBuf;
	 esp32gong.GetWifi().GetGWAddress(sHelp);
	 sprintf(sBuf, "\"ipgateway\":\"%s\",", sHelp);
	 sBody += sBuf;
	 esp32gong.GetWifi().GetNetmask(sHelp);
	 sprintf(sBuf, "\"ipsubnetmask\":\"%s\",", sHelp);
	 sBody += sBuf;
	 }
	 esp32gong.GetWifi().GetMac((__uint8_t*)sHelp);
	 sprintf(sBuf, "\"macaddress\":\"%x:%x:%x:%x:%x:%x\",", sHelp[0], sHelp[1], sHelp[2], sHelp[3], sHelp[4], sHelp[5]);
	 sBody += sBuf;
	 sprintf(sBuf, "\"firmwareversion\":\"%s\"", FIRMWARE_VERSION);
	 sBody += sBuf;
	 sBody += '}';
	 */

	cJSON *json;
	json = cJSON_CreateObject();
	char sbuf[128];

	cJSON_AddStringToObject(json, "esp-idf", esp_get_idf_version());
	cJSON_AddNumberToObject(json, "heap", esp_get_free_heap_size());
	cJSON_AddStringToObject(json, "ssid", esp32gong.GetConfig().msSTASsid.c_str());

	tcpip_adapter_ip_info_t ip_info;
	ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));
	sprintf(sbuf, IPSTR, IP2STR(&ip_info.ip));
	cJSON_AddStringToObject(json, "ipaddress", sbuf);
	sprintf(sbuf, IPSTR, IP2STR(&ip_info.gw));
	cJSON_AddStringToObject(json, "ipgateway", sbuf);
	sprintf(sbuf, IPSTR, IP2STR(&ip_info.netmask));
	cJSON_AddStringToObject(json, "ipsubnetmask", sbuf);
	sprintf(sbuf, IPSTR, IP2STR(&ip_info.gw));
	cJSON_AddStringToObject(json, "ipdns", sbuf);
	cJSON_AddStringToObject(json, "hostname", esp32gong.GetConfig().msHostname.c_str());
	uint8_t mac[6];
	ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA, mac));
	sprintf(sbuf, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	cJSON_AddStringToObject(json, "macaddress", sbuf);
	ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &ip_info));
	sprintf(sbuf, IPSTR, IP2STR(&ip_info.ip));
	cJSON_AddStringToObject(json, "apipaddress", sbuf);

	if (esp32gong.GetConfig().mbAPMode) {
		wifi_sta_list_t apstations;
		if (ESP_OK == esp_wifi_ap_get_sta_list(&apstations)) {
			sprintf(sbuf, "%i", apstations.num);
		} else {
			sprintf(sbuf, "n/a");
		}
		cJSON_AddStringToObject(json, "apconnectedstations", sbuf);

		sprintf(sbuf, "%d.%d.%d.%d", IP2STR((ip4_addr* )&(esp32gong.GetConfig().muLastSTAIpAddress)));
		cJSON_AddStringToObject(json, "lastiptoap", sbuf);
	}
	bool isautoconnect;
	ESP_ERROR_CHECK(esp_wifi_get_auto_connect(&isautoconnect));
	cJSON_AddStringToObject(json, "wifiautoconnect", isautoconnect ? "on" : "off");
	cJSON_AddStringToObject(json, "firmwareversion", FIRMWARE_VERSION);

	sBody = cJSON_PrintUnformatted(json);

	rResponse.AddHeader(HttpResponse::HeaderContentTypeJson);
	rResponse.AddHeader(HttpResponse::HeaderNoCache);
	rResponse.SetRetCode(200);
	return rResponse.Send(sBody.data(), sBody.size());
}

bool DynamicRequestHandler::HandleConfigRequest(std::list<TParam>& params, HttpResponse& rResponse) {

	const char* sWifiMode = NULL;
	const char* sWifiSsid = NULL;
	const char* sWifiPass = NULL;
	const char* sWifiEntPass = NULL;
	const char* sWifiEntUser = NULL;
	const char* sWifiEntCA = NULL;

	std::string sBody;
	ESP_LOGI(LOGTAG, "Config request Handler start");

	std::list<TParam>::iterator it = params.begin();
	while (it != params.end()) {
		ESP_LOGI(LOGTAG, "Config request params: %s=%s", (*it).paramName.c_str(), (*it).paramValue.c_str());

		if ((*it).paramName == "wifimode")
			sWifiMode = (*it).paramValue.data();
		else if ((*it).paramName == "wifissid")
			sWifiSsid = (*it).paramValue.data();
		else if ((*it).paramName == "wifipwd")
			sWifiPass = (*it).paramValue.data();
		else if ((*it).paramName == "wifientpwd")
			sWifiEntPass = (*it).paramValue.data();
		else if ((*it).paramName == "wifientuser")
			sWifiEntUser = (*it).paramValue.data();
		else if ((*it).paramName == "wifientca")
			sWifiEntCA = (*it).paramValue.data();
		it++;
	}

	bool bOk = false;
	if (sWifiSsid) {
		esp32gong.GetConfig().msSTASsid = sWifiSsid;

		if (sWifiMode && (sWifiMode[0] == '2')) { //enterprise wap2
			if (sWifiEntUser && (sWifiEntUser[0] != 0x00)) {
				esp32gong.GetConfig().msSTAENTUser = sWifiEntUser;
				if (sWifiEntCA)
					esp32gong.GetConfig().msSTAENTCA = sWifiEntCA;
				else
					esp32gong.GetConfig().msSTAENTCA.clear();
				if (sWifiEntPass)
					esp32gong.GetConfig().msSTAPass = sWifiEntPass;
				else
					esp32gong.GetConfig().msSTAPass.clear();
				bOk = true;
			}
		} else {
			if (sWifiPass)
				esp32gong.GetConfig().msSTAPass = sWifiPass;
			else
				esp32gong.GetConfig().msSTAPass.clear();
			bOk = true;
		}
	}
	if (bOk) {
		esp32gong.GetConfig().mbAPMode = false;
		esp32gong.GetConfig().Write();
		ESP_LOGI(LOGTAG, "Config WRITTEN");
		mbRestart = true;
		sBody = "Restarting......";
		rResponse.SetRetCode(200);
	} else {
		ESP_LOGE(LOGTAG, "Config NOT OK");
		rResponse.AddHeader("Location: /#!pagewifisettings");
		rResponse.SetRetCode(302);
	}

	rResponse.AddHeader(HttpResponse::HeaderNoCache);
	return rResponse.Send(sBody.data(), sBody.size());
}

void DynamicRequestHandler::CheckForRestart() {

	if (mbRestart) {
		vTaskDelay(100);
		esp_restart();
	}
}

