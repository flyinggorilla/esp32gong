#include "sdkconfig.h"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

#include "Wifi.h"
#include "Config.h"
#include "EspString.h"
#include <esp_event.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_wpa2.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <nvs_flash.h>
#include <lwip/dns.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <esp_netif.h>

static char tag[] = "Wifi";

//NEW
//#include "freertos/event_groups.h"
//static EventGroupHandle_t s_wifi_event_group;

/*
static void setDNSServer(char *ip) {
	ip_addr_t dnsserver;
	ESP_LOGD(tag, "Setting DNS[%d] to %s", 0, ip);
	inet_pton(AF_INET, ip, &dnsserver);
	ESP_LOGD(tag, "ip of DNS is %.8x", *(uint32_t *)&dnsserver);
	dns_setserver(0, &dnsserver);
}
*/

Wifi::Wifi()
{
	muConnectedClients = 0;
	mbConnected = false;
}

void wifiEventHandler(void* ctx, esp_event_base_t base, int32_t id, void* event_data)
{
	return ((Wifi *)ctx)->OnEvent(base, id, event_data);
}

String Wifi::GetLocalAddress()
{
	char sBuf[20];
	GetLocalAddress(sBuf);
	return String(sBuf);
}

void Wifi::GetLocalAddress(char *sBuf)
{
	tcpip_adapter_ip_info_t ip;
	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);
	sprintf(sBuf, "%d.%d.%d.%d", IP2STR(&ip.ip));
}

void Wifi::GetGWAddress(char *sBuf)
{
	tcpip_adapter_ip_info_t ip;
	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);
	sprintf(sBuf, "%d.%d.%d.%d", IP2STR(&ip.gw));
}

void Wifi::GetNetmask(char *sBuf)
{
	tcpip_adapter_ip_info_t ip;
	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);
	sprintf(sBuf, "%d.%d.%d.%d", IP2STR(&ip.netmask));
}

void Wifi::GetMac(__uint8_t uMac[6])
{
	esp_wifi_get_mac(ESP_IF_WIFI_STA, uMac);
}

void Wifi::GetApInfo(int8_t& riRssi, uint8_t& ruChannel)
{
	wifi_ap_record_t info;

	esp_wifi_sta_get_ap_info(&info);
	riRssi = info.rssi;
	ruChannel = info.primary;
}

void Wifi::StartAPMode(String& rsSsid, String& rsPass, String& rsHostname)
{
	msSsid = rsSsid;
	msPass = rsPass;
	msHostname = rsHostname;
	StartAP();
}

void Wifi::StartSTAMode(String& rsSsid, String& rsPass, String& rsHostname)
{
	msSsid = rsSsid;
	msPass = rsPass;
	msHostname = rsHostname;
	Connect();
}

void Wifi::StartSTAModeEnterprise(String& rsSsid, String& rsUser, String& rsPass, String& rsCA, String& rsHostname)
{
	msSsid = rsSsid;
	msUser = rsUser;
	msPass = rsPass;
	msCA = rsCA;
	msHostname = rsHostname;
	Connect();
}

void Wifi::Init() {
	//ESP_ERROR_CHECK(nvs_flash_init());
	//ESP_ERROR_CHECK(esp_netif_init());

	ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifiEventHandler,
                                                        this,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifiEventHandler,
                                                        this,
                                                        &instance_got_ip));		
}

void Wifi::Connect()
{
	ESP_LOGD(tag, "  Connect(<%s><%s><%s><%d>)", msSsid.c_str(), msUser.c_str(), msPass.c_str(), msCA.length());
	ESP_LOGD(tag, "-----------------------");
	ESP_LOGD(tag, "%s", msCA.c_str());
	ESP_LOGD(tag, "-----------------------");
	char sHelp[20];
	GetMac((__uint8_t*)sHelp);
	ESP_LOGD(tag, " macaddress: %x:%x:%x:%x:%x:%x", sHelp[0], sHelp[1], sHelp[2], sHelp[3], sHelp[4], sHelp[5]);
	ESP_LOGD(tag, "-----------------------");

	//NEW
   	//s_wifi_event_group = xEventGroupCreate();

	Init();
	netif = esp_netif_create_default_wifi_sta();
	ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_set_hostname(netif, msHostname.c_str()));

	if (ip.length() > 0 && gw.length() > 0 && netmask.length() > 0)
	{
		tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
		tcpip_adapter_ip_info_t ipInfo;
		inet_pton(AF_INET, ip.c_str(), &ipInfo.ip);
		inet_pton(AF_INET, gw.c_str(), &ipInfo.gw);
		inet_pton(AF_INET, netmask.c_str(), &ipInfo.netmask);
		tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
	}




	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	wifi_config_t config;
	memset(&config, 0, sizeof(config));
	memcpy(config.sta.ssid, msSsid.c_str(), msSsid.length());
	if (!msUser.length())
		memcpy(config.sta.password, msPass.c_str(), msPass.length());
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));

	if (msUser.length())
	{ //Enterprise WPA2
		if (msCA.length())
			esp_wifi_sta_wpa2_ent_set_ca_cert((__uint8_t *)msCA.c_str(), msCA.length());
		esp_wifi_sta_wpa2_ent_set_identity((__uint8_t *)msUser.c_str(), msUser.length());
		esp_wifi_sta_wpa2_ent_set_username((__uint8_t *)msUser.c_str(), msUser.length());
		esp_wifi_sta_wpa2_ent_set_password((__uint8_t *)msPass.c_str(), msPass.length());
		esp_wifi_sta_wpa2_ent_enable(); //!!!!!!!!!!!!!!!!!!!!!!! SHOULD GET CRYPTOFUNC!!!
	}

	ESP_ERROR_CHECK(esp_wifi_start());
}

void Wifi::StartAP()
{
	ESP_LOGD(tag, "  StartAP(<%s>)", msSsid.c_str());
	Init();
	
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_init(&cfg));
    netif = esp_netif_create_default_wifi_ap(); // NEW NEW NEW NEW ESP IDF 4.2
	ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_set_hostname(netif, msHostname.c_str()));
	ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_mode(WIFI_MODE_AP));
	wifi_config_t config;
	memset(&config, 0, sizeof(config));
	memcpy(config.ap.ssid, msSsid.c_str(), msSsid.length());
	config.ap.ssid_len = 0;
	memcpy(config.ap.password, msPass.c_str(), msPass.length());
	config.ap.channel = 0;
	config.ap.authmode = WIFI_AUTH_OPEN;
	config.ap.ssid_hidden = 0;
	config.ap.max_connection = 4;
	config.ap.beacon_interval = 100;
	ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_config(WIFI_IF_AP, &config));
	ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_start());

    //NEW REALLY UNREGISTER??? BETTER MOVE TO INIT / SHUTDOWN METHODS????***********************************
	//************************************************
	/* The event will not be processed after unregister */
    //ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    //ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));	
}

void Wifi::addDNSServer(String& ip)
{
	ip_addr_t dnsserver;
	ESP_LOGD(tag, "Setting DNS[%d] to %s", dnsCount, ip.c_str());
	inet_pton(AF_INET, ip.c_str(), &dnsserver);
	::dns_setserver(dnsCount, &dnsserver);
	dnsCount++;
}

struct in_addr Wifi::getHostByName(String& hostName)
{
	struct in_addr retAddr;
	struct hostent *he = gethostbyname(hostName.c_str());
	if (he == nullptr)
	{
		retAddr.s_addr = 0;
		ESP_LOGD(tag, "Unable to resolve %s - %d", hostName.c_str(), h_errno);
	}
	else
	{
		retAddr = *(struct in_addr *)(he->h_addr_list[0]);
		//ESP_LOGD(tag, "resolved %s to %.8x", hostName, *(uint32_t *)&retAddr);
	}
	return retAddr;
}

void Wifi::setIPInfo(String& ip, String& gw, String& netmask)
{
	this->ip = ip;
	this->gw = gw;
	this->netmask = netmask;
}


void Wifi::OnEvent(esp_event_base_t base, int32_t id, void* event_data)
{
	//esp_err_t rc = ESP_OK;
	if (base == WIFI_EVENT) {
		switch (id)
		{
		case WIFI_EVENT_AP_START:
			ESP_LOGD(tag, "--- WIFI_EVENT_AP_START");
			mbConnected = true;
			break;

		case WIFI_EVENT_AP_STOP:
			ESP_LOGD(tag, "--- WIFI_EVENT_AP_STOP");
			mbConnected = false;
			break;
		case WIFI_EVENT_AP_STACONNECTED:
			muConnectedClients++;
			ESP_LOGD(tag, "--- WIFI_EVENT_AP_STACONNECTED - %d clients", muConnectedClients);
			//if (mpStateDisplay)
			//	mpStateDisplay->SetConnected(true, this);
			break;
		case WIFI_EVENT_AP_STADISCONNECTED:
			if (muConnectedClients)
				muConnectedClients--;
			ESP_LOGD(tag, "--- WIFI_EVENT_AP_STADISCONNECTED - %d clients", muConnectedClients);
			//if (!muConnectedClients && mpStateDisplay)
			//	mpStateDisplay->SetConnected(false, this);
			break;
		case WIFI_EVENT_STA_CONNECTED:
			ESP_LOGD(tag, "--- WIFI_EVENT_STA_CONNECTED");
			break;
		case WIFI_EVENT_STA_DISCONNECTED:
			ESP_LOGD(tag, "--- WIFI_EVENT_STA_DISCONNECTED");
			mbConnected = false;
			//if (mpStateDisplay)
			//	mpStateDisplay->SetConnected(false, this);
			esp_wifi_connect();
			break;
		case WIFI_EVENT_STA_START: {
			ESP_LOGD(tag, "--- WIFI_EVENT_STA_START");
			//ESP_LOGD(tag, "SETTING HOSTNAME: %s", msHostname.c_str() == NULL ? "NULL" : msHostname.c_str());
			//ESP_ERROR_CHECK(tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, msHostname.c_str()));
			esp_err_t errConnect = esp_wifi_connect();
			switch (errConnect) {
				case 0 : 
					break;
				case ESP_ERR_WIFI_SSID : 
					ESP_LOGW(tag, "Invalid SSID: %s", msSsid.c_str() == NULL ? "NULL" : msSsid.c_str());
					break;
				default: 
					ESP_LOGE(tag, "Error connecting Wifi: %s", esp_err_to_name(errConnect));
					break;
			}
			break;
		}
		case WIFI_EVENT_STA_STOP:
			ESP_LOGD(tag, "--- WIFI_EVENT_STA_STOP");
			break;
		default:
			break;
		}
	} else if (base == IP_EVENT) {
		switch(id) {
		case IP_EVENT_STA_GOT_IP:
			ESP_LOGD(tag, "--- IP_EVENT_STA_GOT_IP");
			mbConnected = true;
			//if (mpStateDisplay)
			//	mpStateDisplay->SetConnected(true, this);
			
			//EXAMPLE CODE
        	//ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        	//ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));

			tcpip_adapter_ip_info_t ip;
			tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);
			if (mpConfig)
				mpConfig->muLastSTAIpAddress = ip.ip.addr;
			break;
		default:
			break;
		}
    }
}
