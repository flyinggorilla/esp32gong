#ifndef MAIN_ESP32GONG_H_
#define MAIN_ESP32GONG_H_

#include "Config.h"

#define FIRMWARE_VERSION __DATE__ " " __TIME__

class Esp32Gong {
public:
	Esp32Gong();
	virtual ~Esp32Gong();

	void Start();

	void TaskWebServer();
	void TaskResetButton();
	void TaskDnsServer();
	void TaskTestWebClient();

	void IndicateApiCall() 	{ mbApiCallReceived = true; };
	void Restart(int seconds);
	Config& GetConfig() { return mConfig; }

private:
	bool mbButtonPressed;
	bool mbApiCallReceived;
	Config mConfig;

private:


};




#endif /* MAIN_ESP32GONG_H_ */
