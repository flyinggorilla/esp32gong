#include "Esp32Gong.hpp"

#include "sdkconfig.h"
//#define _GLIBCXX_USE_C99

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <stdio.h>
#include <string>
#include "math.h"
#include "string.h"
#include "wavdata.h"
#include "I2SPlayer.hpp"
#include "SpiffsFileSystem.hpp"
#include "WebServer.hpp"
#include "Wifi.hpp"
#include "Config.hpp"
#include "DnsSrv.hpp"
#include "Ota.hpp"

#define ONBOARDLED_GPIO GPIO_NUM_5  // GPIO5 on Sparkfun ESP32 Thing
#define LOGTAG "main"

I2SPlayer musicPlayer;
WebServer webServer;
Esp32Gong esp32gong;
SpiffsFileSystem spiffsFileSystem;
Wifi wifi;
DnsSrv dnsServer;

// Wav* wav = NULL;
Esp32Gong::Esp32Gong() {
//	wav = NULL;
}

Esp32Gong::~Esp32Gong() {
//	delete wav;
}

extern "C" {
void app_main();
}

void app_main() {
	nvs_flash_init();
	tcpip_adapter_init();
	esp32gong.Start();
}

//===========================================

void task_function_webserver(void *pvParameter) {
	((Esp32Gong*) pvParameter)->TaskWebServer();
	vTaskDelete(NULL);
}

void task_function_resetbutton(void *pvParameter) {
	((Esp32Gong*) pvParameter)->TaskResetButton();
	vTaskDelete(NULL);
}

void task_function_dnsserver(void *pvParameter) {
	((Esp32Gong*) pvParameter)->TaskDnsServer();
	vTaskDelete(NULL);
}

void task_function_restart(void* user_data) {
	ESP_LOGI(LOGTAG, "Restarting in 2secs....");
	vTaskDelay(*((int*)user_data)*1000 / portTICK_PERIOD_MS);
	esp_restart();
}

void Esp32Gong::Restart(int seconds) {
	xTaskCreate(&task_function_restart, "restartTask", 2048, &seconds, 5, NULL);
}

//----------------------------------------------------------------------------------------

void Esp32Gong::Start() {

	ESP_LOGI(LOGTAG, "Welcome to Bernd's ESP32 Gong");
	ESP_LOGI(LOGTAG, "ESP-IDF version %s", esp_get_idf_version());
	ESP_LOGI(LOGTAG, "Firmware version %s", FIRMWARE_VERSION);
	musicPlayer.init();
	musicPlayer.prepareWav(wavdata_h, sizeof(wavdata_h));
	//musicPlayer.playAsync();

	mbButtonPressed = !gpio_get_level(GPIO_NUM_0);
	mConfig.Read();

	gpio_pad_select_gpio(10);
	gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
	gpio_set_pull_mode(GPIO_NUM_0, GPIO_PULLUP_ONLY);

	gpio_pad_select_gpio((gpio_num_t) ONBOARDLED_GPIO);
	gpio_set_direction((gpio_num_t) ONBOARDLED_GPIO, (gpio_mode_t) GPIO_MODE_OUTPUT);

	xTaskCreate(&task_function_webserver, "Task_WebServer", 4096, this, 5, NULL);
	xTaskCreate(&task_function_resetbutton, "Task_ResetButton", 4096, this, 5, NULL);

	ESP_LOGI(LOGTAG, "CONFIG HOSTNAME: %s", mConfig.msHostname.c_str() == NULL ? "NULL" : mConfig.msHostname.c_str());

	if (mConfig.mbAPMode) {
		if (mConfig.muLastSTAIpAddress) {
			char sBuf[16];
			sprintf(sBuf, "%d.%d.%d.%d", IP2STR((ip4_addr* )&mConfig.muLastSTAIpAddress));
			ESP_LOGD(LOGTAG, "Last IP when connected to AP: %d : %s", mConfig.muLastSTAIpAddress, sBuf);
		}
		wifi.StartAPMode(mConfig.msAPSsid, mConfig.msAPPass, mConfig.msHostname);
		// start DNS server to always redirect any domain to 192.168.4.1
		xTaskCreate(&task_function_dnsserver, "Task_DnsServer", 16000, this, 5, NULL);
	} else {
		if (mConfig.msSTAENTUser.length())
			wifi.StartSTAModeEnterprise(mConfig.msSTASsid, mConfig.msSTAENTUser, mConfig.msSTAPass, mConfig.msSTAENTCA, mConfig.msHostname);
		else
			wifi.StartSTAMode(mConfig.msSTASsid, mConfig.msSTAPass, mConfig.msHostname);
		const char* hostname;
		tcpip_adapter_get_hostname(TCPIP_ADAPTER_IF_STA, &hostname);
		ESP_LOGI(LOGTAG, "Station hostname: %s", hostname);
		tcpip_adapter_get_hostname(TCPIP_ADAPTER_IF_AP, &hostname);
		ESP_LOGI(LOGTAG, "AP hostname: %s", hostname);
		ESP_LOGI(LOGTAG, "MDNS feature is disabled - no use found for it so far -- SSDP more interesting");
		//wifi.StartMDNS();
	}

	while(!wifi.IsConnected()) {
		vTaskDelay(100/portTICK_PERIOD_MS);
	}
	Ota ota;
	ota.update("http://www.msftconnecttest.com/connecttest.txt");
	//ota.update("https://github.com/flyinggorilla/esp32gong/blob/master/README.md");

}

void Esp32Gong::TaskWebServer() {
	webServer.Start(80);
}

void Esp32Gong::TaskDnsServer() {
	dnsServer.start();
}

void Esp32Gong::TaskResetButton() {
	int level = 0;
	int ticks = 0;



	while (1) {
		if (wifi.IsConnected() && mbApiCallReceived) {
			gpio_set_level((gpio_num_t) ONBOARDLED_GPIO, (gpio_mode_t) level);
			level = !level;
			ticks = 0;
		} else {
			if (ticks > 1) { // blink half speed
				level = !level;
				ticks = 0;
			}
		}
		ticks++;

		gpio_set_level((gpio_num_t) ONBOARDLED_GPIO, (gpio_mode_t) level);
		vTaskDelay(500 / portTICK_PERIOD_MS);

		if (!gpio_get_level(GPIO_NUM_0)) {
			if (!mbButtonPressed) {
				ESP_LOGI(LOGTAG, "Factory settings button pressed... rebooting into Access Point mode.");
				mConfig.ToggleAPMode();
				mConfig.Write();
				esp_restart();
			}
		} else {
			mbButtonPressed = false;
		}

		vTaskDelay(1);
	}
}

//-----------------------------------------------------------------------------------------

