/*
 * WiFi.h
 *
 *  Created on: Feb 25, 2017
 *      Author: kolban
 */

#ifndef MAIN_WIFI_H_
#define MAIN_WIFI_H_
#include "sdkconfig.h"
#include "esp_wifi.h"
#include "esp_err.h"
#include <freertos/event_groups.h>

#include <string>
#include <vector>

class Config;

class Wifi {

public:
	Wifi();

	void SetConfig(Config* pConfig) {
		mpConfig = pConfig;
	}
	;
	void GetLocalAddress(char* sBuf);
	void GetGWAddress(char* sBuf);
	void GetNetmask(char* sBuf);
	void GetMac(__uint8_t uMac[6]);

	void StartAPMode(std::string& rsSsid, std::string& rsPass, std::string& rsHostname);
	void StartSTAMode(std::string& rsSsid, std::string& rsPass, std::string& rsHostname);
	void StartSTAModeEnterprise(std::string& rsSsid, std::string& rsUser, std::string& rsPass, std::string& rsCA, std::string& rsHostname);

	bool IsConnected() {
		return mbConnected;
	}
	;
	void addDNSServer(std::string ip);
	struct in_addr getHostByName(std::string hostName);
	void setIPInfo(std::string ip, std::string gw, std::string netmask);

	esp_err_t OnEvent(system_event_t *event);

	bool waitForConnection(unsigned int timeoutSecs = 60); // default wait 60 seconds

	bool StartMDNS();
	void TaskMDNS();

private:
	void Connect();
	void StartAP();

private:
	Config* mpConfig;

	std::string ip;
	std::string gw;
	std::string netmask;

	__uint8_t muMode;
	std::string msSsid;
	std::string msPass;
	std::string msUser;
	std::string msCA;
	std::string msHostname;

	__uint8_t muConnectedClients;bool mbConnected;

	int dnsCount = 0;
	char *dnsServer = nullptr;

	EventGroupHandle_t wifi_event_group;
	const int WIFI_CONNECTED_BIT = BIT0;

};

#endif /* MAIN_WIFI_H_ */
