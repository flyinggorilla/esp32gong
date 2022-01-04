/*
 * Wifi.h
 *
 */

#ifndef MAIN_WIFI_H_
#define MAIN_WIFI_H_
#include "sdkconfig.h"
#include "esp_wifi.h"
#include "esp_err.h"
#include "EspString.h"
#include <vector>

class Config;
//class StateDisplay;

class Wifi {

public:
	Wifi();

	void SetConfig(Config* pConfig)						{ mpConfig = pConfig; };
	//void SetStateDisplay(StateDisplay* pStateDisplay)  	{ mpStateDisplay = pStateDisplay; };

	String GetLocalAddress();
	void GetLocalAddress(char* sBuf);
	void GetGWAddress(char* sBuf);
	ip4_addr_t GetGWAddress();
	void GetNetmask(char* sBuf);
	void GetMac(__uint8_t uMac[6]);
	void GetApInfo(int8_t& riRssi, uint8_t& ruChannel);

	void StartAPMode(String& rsSsid, String& rsPass, String& rsHostname);
	void StartSTAMode(String& rsSsid, String& rsPass, String& rsHostname);
	void StartSTAModeEnterprise(String& rsSsid, String& rsUser, String& rsPass, String& rsCA, String& rsHostname);


	bool IsConnected() { return mbConnected; };
	void addDNSServer(String& ip);
	struct in_addr getHostByName(String& hostName);
	void setIPInfo(String& ip, String& gw, String& netmask);

	[[deprecated]]
	esp_err_t OnEvent(system_event_t *event);

	void OnEvent(esp_event_base_t base, int32_t id, void* event_data);
	

private:
	void Connect();
	void StartAP();
	void Init();

private:
	Config* mpConfig;
	//StateDisplay* mpStateDisplay;

	String      ip;
	String      gw;
	String      netmask;

	__uint8_t        muMode;
	String      msSsid;
	String      msPass;
	String      msUser;
	String      msCA;
	String 	 msHostname;

	__uint8_t 		muConnectedClients;
	bool 			mbConnected;

	int dnsCount=0;
	char *dnsServer = nullptr;

	esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
	esp_netif_t* netif = nullptr;

};

#endif /* MAIN_WIFI_H_ */
