#ifndef MAIN_CONFIG_H_
#define MAIN_CONFIG_H_

#include "nvs.h"
#include <string>

class Config {
public:
	Config();
	virtual ~Config();

	bool Read();
	bool Write();

	void ToggleAPMode() { mbAPMode = !mbAPMode; };

private:
	bool ReadString(nvs_handle h, const char* sKey, std::string& rsValue);
	bool ReadBool(nvs_handle h, const char* sKey, bool& rbValue);
	bool WriteString(nvs_handle h, const char* sKey, std::string& rsValue);
	bool WriteBool(nvs_handle h, const char* sKey, bool bValue);


public:
	bool mbAPMode;
	std::string msAPSsid;
	std::string msAPPass;
	std::string msSTASsid;
	std::string msSTAPass;
	std::string msSTAENTUser;
	std::string msSTAENTCA;
	std::string msHostname;

	bool mbWebServerUseSsl;
	__uint16_t muWebServerPort;

	__uint32_t muLastSTAIpAddress;
};

#endif /* MAIN_CONFIG_H_ */
