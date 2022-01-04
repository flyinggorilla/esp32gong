#ifndef MAIN_PINGWATCHDOG_H_
#define MAIN_PINGWATCHDOG_H_

#include <ping/ping_sock.h>
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"

class PingWatchdog {
public:
	PingWatchdog();
	virtual ~PingWatchdog();

    void Start(ip4_addr_t ipAddress);
	

private:
	esp_ping_handle_t mhPingSession;
	esp_ping_config_t mPingConfig;
	esp_ping_callbacks_t mPingCallbacks;
	unsigned int muSuccess = 0;

    //main loop run by the task
    void PingWatchdogTask();
    friend void fPingWatchdogTask(void *pvParameter);
	friend void onPingSuccess(esp_ping_handle_t hdl, void *args);
	friend void onPingTimeout(esp_ping_handle_t hdl, void *args);
	friend void onPingEnd(esp_ping_handle_t hdl, void *args);

	void Success();
	void Timeout();
	void End();

};

#endif 
