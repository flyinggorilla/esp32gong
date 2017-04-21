#include "DynamicRequestHandler.h"

#include "esp_system.h"
#include <esp_log.h>
#include <cJSON.h>
#include "Config.h"
#include "Esp32Gong.h"
#include "I2SPlayer.h"
#include "Wifi.h"

extern Wifi wifi;
extern Esp32Gong esp32gong;
extern I2SPlayer musicPlayer;

DynamicRequestHandler::DynamicRequestHandler() {
	mbRestart = false;
}

DynamicRequestHandler::~DynamicRequestHandler() {

}

__uint8_t DynamicRequestHandler::HandleApiRequest(std::list<TParam>& params, std::string& body) {

	std::list<TParam>::iterator it = params.begin();
	while (it != params.end()) {
		if ((*it).paramName == "gong") {
			musicPlayer.setVolume(25);
			musicPlayer.playAsync();
			body = "<html><body>api call - lets play music</html></body>";
		}
		it++;
	}

	return 200;
}


__uint8_t DynamicRequestHandler::HandleInfoRequest(std::list<TParam>& params, std::string& body) {

	/*char sBuf[100];
	 char sHelp[20];

	 body = "{";
	 sprintf(sBuf, "\"apmode\":\"%d\",", esp32gong->GetConfig().mbAPMode);
	 body += sBuf;
	 sprintf(sBuf, "\"heap\":\"%d Bytes\",", esp_get_free_heap_size());
	 body += sBuf;
	 sprintf(sBuf, "\"ssid\":\"%s\",", esp32gong.GetConfig().msSTASsid.data());
	 body += sBuf;

	 if (esp32gong->GetConfig().mbAPMode){
	 sprintf(sBuf, "\"lastiptoap\":\"%d.%d.%d.%d\",", IP2STR((ip4_addr*)&(esp32gong.GetConfig().muLastSTAIpAddress)));
	 body += sBuf;
	 }
	 else{
	 esp32gong.GetWifi().GetLocalAddress(sHelp);
	 sprintf(sBuf, "\"ipaddress\":\"%s\",", sHelp);
	 body += sBuf;
	 esp32gong.GetWifi().GetGWAddress(sHelp);
	 sprintf(sBuf, "\"ipgateway\":\"%s\",", sHelp);
	 body += sBuf;
	 esp32gong.GetWifi().GetNetmask(sHelp);
	 sprintf(sBuf, "\"ipsubnetmask\":\"%s\",", sHelp);
	 body += sBuf;
	 }
	 esp32gong.GetWifi().GetMac((__uint8_t*)sHelp);
	 sprintf(sBuf, "\"macaddress\":\"%x:%x:%x:%x:%x:%x\",", sHelp[0], sHelp[1], sHelp[2], sHelp[3], sHelp[4], sHelp[5]);
	 body += sBuf;
	 sprintf(sBuf, "\"firmwareversion\":\"%s\"", FIRMWARE_VERSION);
	 body += sBuf;

	 bool isautoconnect;
	 ESP_ERROR_CHECK(esp_wifi_get_auto_connect(&isautoconnect));
	 sprintf(sBuf, "\"wifiautoconnect\": \"%s\",", isautoconnect ? "on" : "off");
	 body += sBuf;
	 body += '}';*/

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

	body = cJSON_PrintUnformatted(json);

	return 200;
}

__uint8_t DynamicRequestHandler::HandleConfigRequest(std::list<TParam>& params, std::string& body) {

	const char* sWifiSsid = NULL;
	const char* sWifiPass = NULL;
	const char* sWifiUser = NULL;
	const char* sWifiCA = NULL;

	std::list<TParam>::iterator it = params.begin();
	while (it != params.end()) {

		if ((*it).paramName == "wifissid")
			sWifiSsid = (*it).paramValue.data();
		else if ((*it).paramName == "wifipwd")
			sWifiPass = (*it).paramValue.data();
		else if ((*it).paramName == "wifiuser")
			sWifiUser = (*it).paramValue.data();
		else if ((*it).paramName == "wifica")
			sWifiCA = (*it).paramValue.data();
		it++;
	}
	if (sWifiSsid && sWifiPass) {
		esp32gong.GetConfig().msSTASsid = sWifiSsid;
		esp32gong.GetConfig().msSTAPass = sWifiPass;
		if (sWifiUser)
			esp32gong.GetConfig().msSTAENTUser = sWifiUser;
		else
			esp32gong.GetConfig().msSTAENTUser.clear();
		if (sWifiCA) {
			//ESP_LOGD("DynamicRequestHandler", "<%s>", sWifiCA);
			esp32gong.GetConfig().msSTAENTCA = sWifiCA;
		} else
			esp32gong.GetConfig().msSTAENTCA.clear();
		esp32gong.GetConfig().mbAPMode = false;
		esp32gong.GetConfig().Write();
		mbRestart = true;
		body = "<html><body>New Wifi credentials successfully set! rebooting now....... browser refreshs in 10 seconds to http://";
		body += esp32gong.GetConfig().msHostname;
		body +="</html></body>";

	}

	return 200;
}

void DynamicRequestHandler::CheckForRestart() {

	if (mbRestart) {
		vTaskDelay(100);
		esp_restart();
	}
}

