#include "Wifi.hpp"
#include "Config.hpp"

#include <esp_event.h>
#include <esp_event_loop.h>
#include <esp_interface.h>
#include <esp_log.h>
#include <esp_wifi_types.h>
#include <esp_wpa2.h>
#include <freertos/portmacro.h>
#include <freertos/projdefs.h>
#include <freertos/task.h>
#include <mdns.h>
#include <nvs_flash.h>
#include <string.h>
#include <tcpip_adapter.h>
#include <cstdio>
//#include <sys/socket.h>
#include "../../esp-idf/components/lwip/include/lwip/lwip/dns.h" //ODD INCLUSION DUE TO ECLIPSE
#include "../../esp-idf/components/lwip/include/lwip/lwip/netdb.h" //ODD INCLUSION DUE TO ECLIPSE
#include "../../esp-idf/components/lwip/include/lwip/lwip/sockets.h" //ODD INCLUSION DUE TO ECLIPSE
#include "../../esp-idf/components/lwip/include/lwip/lwip/ip_addr.h"

struct in_addr;

static const char LOGTAG[] = "Wifi";

Wifi::Wifi() {
	muConnectedClients = 0;
	mbConnected = false;
	wifi_event_group = xEventGroupCreate();
}

esp_err_t eventHandler(void *ctx, system_event_t *event) {
	return ((Wifi*) ctx)->OnEvent(event);
}

void Wifi::GetLocalAddress(char* sBuf) {
	tcpip_adapter_ip_info_t ip;
	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);
	sprintf(sBuf, "%d.%d.%d.%d", IP2STR(&ip.ip));
}

void Wifi::GetGWAddress(char* sBuf) {
	tcpip_adapter_ip_info_t ip;
	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);
	sprintf(sBuf, "%d.%d.%d.%d", IP2STR(&ip.gw));
}
void Wifi::GetNetmask(char* sBuf) {
	tcpip_adapter_ip_info_t ip;
	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);
	sprintf(sBuf, "%d.%d.%d.%d", IP2STR(&ip.netmask));
}

void Wifi::GetMac(__uint8_t uMac[6]) {
	esp_wifi_get_mac(ESP_IF_WIFI_STA, uMac);
}

void Wifi::StartAPMode(std::string& rsSsid, std::string& rsPass, std::string& rsHostname) {
	msSsid = rsSsid;
	msPass = rsPass;
	msHostname = rsHostname;
	StartAP();
}

void Wifi::StartSTAMode(std::string& rsSsid, std::string& rsPass, std::string& rsHostname) {
	msSsid = rsSsid;
	msPass = rsPass;
	msHostname = rsHostname;
	Connect();
}

void Wifi::StartSTAModeEnterprise(std::string& rsSsid, std::string& rsUser, std::string& rsPass, std::string& rsCA, std::string& rsHostname) {
	msSsid = rsSsid;
	msUser = rsUser;
	msPass = rsPass;
	msCA = rsCA;
	msHostname = rsHostname;
	Connect();
}

void Wifi::Connect() {
	ESP_LOGD(LOGTAG, "  Connect(<%s><%s><%s><%d>)", msSsid.data(), msUser.data(), msPass.data(), msCA.length());

	if (ip.length() > 0 && gw.length() > 0 && netmask.length() > 0) {
		tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
		tcpip_adapter_ip_info_t ipInfo;
		inet_pton(AF_INET, ip.data(), &ipInfo.ip);
		inet_pton(AF_INET, gw.data(), &ipInfo.gw);
		inet_pton(AF_INET, netmask.data(), &ipInfo.netmask);
		tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
	}

	esp_event_loop_init(eventHandler, this);
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT()
	;
	esp_wifi_init(&cfg);
	esp_wifi_set_storage(WIFI_STORAGE_RAM);
	esp_wifi_set_mode(WIFI_MODE_STA);
	wifi_config_t config;
	memset(&config, 0, sizeof(config));
	memcpy(config.sta.ssid, msSsid.data(), msSsid.length());
	if (!msUser.length())
		memcpy(config.sta.password, msPass.data(), msPass.length());
	esp_wifi_set_config(WIFI_IF_STA, &config);

	if (msUser.length()) { //Enterprise WPA2
		if (msCA.length())
			esp_wifi_sta_wpa2_ent_set_ca_cert((__uint8_t *) msCA.data(), msCA.length());
		esp_wifi_sta_wpa2_ent_set_identity((__uint8_t *) msUser.data(), msUser.length());
		esp_wifi_sta_wpa2_ent_set_username((__uint8_t *) msUser.data(), msUser.length());
		esp_wifi_sta_wpa2_ent_set_password((__uint8_t *) msPass.data(), msPass.length());
		esp_wifi_sta_wpa2_ent_enable();
	}

	esp_wifi_start();
	ESP_LOGI(LOGTAG, "SETTING HOSTNAME: %s", msHostname.c_str() == NULL ? "NULL" : msHostname.c_str());
	ESP_ERROR_CHECK(tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, msHostname.c_str()));
	esp_wifi_connect();
}

bool Wifi::waitForConnection(unsigned int timeoutSecs) {
	if (timeoutSecs == 0) {
		timeoutSecs = portMAX_DELAY; // portMAX_DELAY is the constant for max delay
	}
	return WIFI_CONNECTED_BIT && xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT,
	pdFALSE, pdFALSE, timeoutSecs * 1000 / portTICK_PERIOD_MS);

}

void Wifi::StartAP() {
	ESP_LOGI(LOGTAG, "Starting AP: SSID=%s", msSsid.data());
	nvs_flash_init();
	tcpip_adapter_init();

	esp_event_loop_init(eventHandler, this);
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT()
	;
	esp_wifi_init(&cfg);
	esp_wifi_set_storage(WIFI_STORAGE_RAM);
	esp_wifi_set_mode(WIFI_MODE_AP);
	wifi_config_t config;
	memset(&config, 0, sizeof(config));
	memcpy(config.ap.ssid, msSsid.data(), msSsid.length());
	config.ap.ssid_len = 0;
	memcpy(config.ap.password, msPass.data(), msPass.length());
	config.ap.channel = 0;
	config.ap.authmode = WIFI_AUTH_OPEN;
	config.ap.ssid_hidden = 0;
	config.ap.max_connection = 4;
	config.ap.beacon_interval = 100;
	esp_wifi_set_config(WIFI_IF_AP, &config);
	esp_wifi_start();
}

void Wifi::addDNSServer(std::string ip) {
	ip_addr_t dnsserver;
	ESP_LOGD(LOGTAG, "Setting DNS[%d] to %s", dnsCount, ip.c_str());
	inet_pton(AF_INET, ip.c_str(), &dnsserver);
	::dns_setserver(dnsCount, &dnsserver);
	dnsCount++;
}

struct in_addr Wifi::getHostByName(std::string hostName) {
	struct in_addr retAddr;
	struct hostent *he = gethostbyname(hostName.c_str());
	if (he == nullptr) {
		retAddr.s_addr = 0;
		ESP_LOGD(LOGTAG, "Unable to resolve %s - %d", hostName.c_str(), h_errno);
	} else {
		retAddr = *(struct in_addr *) (he->h_addr_list[0]);
		//ESP_LOGD(tag, "resolved %s to %.8x", hostName, *(uint32_t *)&retAddr);

	}
	return retAddr;
}

void Wifi::setIPInfo(std::string ip, std::string gw, std::string netmask) {
	this->ip = ip;
	this->gw = gw;
	this->netmask = netmask;
}

esp_err_t Wifi::OnEvent(system_event_t *event) {

	esp_err_t rc = ESP_OK;
	switch (event->event_id) {

	case SYSTEM_EVENT_AP_START:
		ESP_LOGD(LOGTAG, "--- SYSTEM_EVENT_AP_START")
		;
		mbConnected = true;
		break;
	case SYSTEM_EVENT_AP_STOP:
		ESP_LOGD(LOGTAG, "--- SYSTEM_EVENT_AP_STOP")
		;
		mbConnected = false;
		break;
	case SYSTEM_EVENT_AP_STACONNECTED:
		muConnectedClients++;
		ESP_LOGD(LOGTAG, "--- SYSTEM_EVENT_AP_STACONNECTED - %d clients", muConnectedClients)
		;
		/* enable ipv6 */ //TODO IS THIS NEEDED FOR MDNS??
		tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_AP);
		break;
	case SYSTEM_EVENT_AP_STADISCONNECTED:
		if (muConnectedClients)
			muConnectedClients--;
		ESP_LOGD(LOGTAG, "--- SYSTEM_EVENT_AP_STADISCONNECTED - %d clients", muConnectedClients)
		;
		break;

	case SYSTEM_EVENT_STA_CONNECTED:
		ESP_LOGD(LOGTAG, "--- SYSTEM_EVENT_STA_CONNECTED")
		;
		/* enable ipv6 -- NEEDED for MDNS ???*/
		tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		ESP_LOGD(LOGTAG, "--- SYSTEM_EVENT_STA_DISCONNECTED")
		;
		xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
		mbConnected = false;

		/* This is a workaround as ESP32 WiFi libs don't currently
		 auto-reassociate. */
		if (esp_wifi_connect() != ESP_OK) {
			ESP_LOGI(LOGTAG, "reconnecting wifi failed");
		}
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		ESP_LOGD(LOGTAG, "--- SYSTEM_EVENT_STA_GOT_IP")
		;
		mbConnected = true;

		tcpip_adapter_ip_info_t ip;
		tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);
		if (mpConfig)
			mpConfig->muLastSTAIpAddress = ip.ip.addr;
		// set signal that IP address is now available -- so to e.g. start webserver from this point on
		xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_STA_START:
		ESP_LOGD(LOGTAG, "--- SYSTEM_EVENT_STA_START")
		;
		break;
	case SYSTEM_EVENT_STA_STOP:
		ESP_LOGD(LOGTAG, "--- SYSTEM_EVENT_STA_STOP")
		;
		break;
	default:
		break;
	}
	return rc;
}

void task_function_mdns(void *pvParameter) {
	((Wifi*) pvParameter)->TaskMDNS();
	vTaskDelete(NULL);
}

bool Wifi::StartMDNS() {
	xTaskCreate(&task_function_mdns, "mdns_task", 4096, this, 5, NULL);
	return true;
}

void Wifi::TaskMDNS() {
	mdns_server_t * mdns = NULL;
	esp_err_t err = -1;
	while (err) {
		/* Wait for the callback to set the CONNECTED_BIT in the
		 event group.
		 */
		esp_err_t err = mdns_init(TCPIP_ADAPTER_IF_STA, &mdns);
		if (err) {
			ESP_LOGE(LOGTAG, "Failed starting MDNS: %u", err);
			ESP_LOGI(LOGTAG, "Retrying in 5 seconds!");
			vTaskDelay(5000 / portTICK_PERIOD_MS);
		}
	}
	ESP_ERROR_CHECK(mdns_set_hostname(mdns, "Esp32Gong")); //TODO define hostname!!!!!!!
	ESP_ERROR_CHECK(mdns_set_instance(mdns, "Esp32Gong"));

	//TODO
	const char * testTxtData[4] = { "board=esp32", "tcp_check=dummy-test", "ssh_upload=no", "auth_upload=no" };

	//service types http://agnat.github.io/node_mdns/user_guide.html
	ESP_ERROR_CHECK(mdns_service_add(mdns, "_http", "_tcp", 80));
	ESP_ERROR_CHECK(mdns_service_add(mdns, "_https", "_tcp", 443));
	ESP_ERROR_CHECK(mdns_service_instance_set(mdns, "_http", "_tcp", "ESP32 WebServer"));
	ESP_ERROR_CHECK(mdns_service_txt_set(mdns, "_esp32gong", "_txt", 4, testTxtData));

}

