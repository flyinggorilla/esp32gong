#include "Esp32Gong.h"

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

#include "Config.h"
#include "DnsSrv.h"
#include "I2SPlayer.h"
#include "temperature.h"
#include "wavdata.h"
#include "WebServer.h"
#include "Wifi.h"
#include "FileSystem.h"
#include "Url.h"
#include "WebClient.h"
#include "Ota.h"

#define ONBOARDLED_GPIO GPIO_NUM_5  // GPIO5 on Sparkfun ESP32 Thing
#define LOGTAG "main"

I2SPlayer musicPlayer;
WebServer webServer;
Esp32Gong esp32gong;
FileSystem fileSystem;
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

void task_test_webclient(void *pvParameter) {
	((Esp32Gong*) pvParameter)->TaskTestWebClient();
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

//	xTaskCreate(&task_function_webserver, "Task_WebServer", 8192*2, this, 5, NULL);
	xTaskCreate(&task_function_webserver, "Task_WebServer", 8192, this, 5, NULL);
	xTaskCreate(&task_function_resetbutton, "Task_ResetButton", 2048, this, 5, NULL);

	ESP_LOGI(LOGTAG, "CONFIG HOSTNAME: %s", mConfig.msHostname.c_str() == NULL ? "NULL" : mConfig.msHostname.c_str());

	if (mConfig.mbAPMode) {
		if (mConfig.muLastSTAIpAddress) {
			char sBuf[16];
			sprintf(sBuf, "%d.%d.%d.%d", IP2STR((ip4_addr* )&mConfig.muLastSTAIpAddress));
			ESP_LOGD(LOGTAG, "Last IP when connected to AP: %d : %s", mConfig.muLastSTAIpAddress, sBuf);
		}
		wifi.StartAPMode(mConfig.msAPSsid, mConfig.msAPPass, mConfig.msHostname);
		// start DNS server to always redirect any domain to 192.168.4.1
//		xTaskCreate(&task_function_dnsserver, "Task_DnsServer", 8192, this, 5, NULL);
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

	//xTaskCreate(&task_test_webclient, "Task_TestWebClient", 8192, this, 5, NULL);
	//ESP_LOGI(LOGTAG, "********************** OTA MAIN VERSION *********************");
	//Ota ota;
    //ota.UpdateFirmware("https://surpro4:9999/getfirmware");
	//ESP_LOGI(LOGTAG, "********************** OTA THREAD VERSION PINNED TO CORE *********************");
	//Ota::StartUpdateFirmwareTask();


}

void Esp32Gong::TaskTestWebClient() {
	Url url;
	WebClient webClient;
	//url.Selftest();

	/* url.Parse("http://www.msftconnecttest.com/connecttest.txt");
    webClient.Prepare(&url);
    webClient.AddHttpHeaderCStr("Connection: close");
    webClient.AddHttpHeaderCStr("Test1: testVal1");
    webClient.AddHttpHeaderCStr("Test2: testVal2; testVal3");
  	ESP_LOGW(LOGTAG, "Msftconnecttest Execute#1");
    if (!webClient.HttpGet()) {
      	ESP_LOGE(LOGTAG, "Error requesting: %s", url.GetUrl().c_str());
    }

  	ESP_LOGW(LOGTAG, "Msftconnecttest Execute#2");
    if (!webClient.HttpGet()) {
      	ESP_LOGE(LOGTAG, "Error requesting: %s", url.GetUrl().c_str());
    }
  	ESP_LOGW(LOGTAG, "Msftconnecttest Execute#3");
    if (!webClient.HttpGet()) {
      	ESP_LOGE(LOGTAG, "Error requesting: %s", url.GetUrl().c_str());
    }


  	ESP_LOGW(LOGTAG, "SSL CHECK: https://www.howsmyssl.com/a/check");
	url.Parse("https://www.howsmyssl.com/a/check");
    if (!webClient.Prepare(&url)) {
    	ESP_LOGE(LOGTAG, "Error in HttpPrepareGet()")
    }
  	ESP_LOGW(LOGTAG, "SSL CHECK BEFORE HTTPEXECUTE: https://www.howsmyssl.com/a/check");

    if (!webClient.HttpGet()) {
      	ESP_LOGE(LOGTAG, "Error in HttpExecute()")
    }

  	ESP_LOGW(LOGTAG, "SSL CHECK AFTER  HTTPEXECUTE: https://www.howsmyssl.com/a/check");

  	ESP_LOGW(LOGTAG, "BEGOIN https://httpbin.org/headers");
	url.Parse("http://httpbin.org/headers");
    webClient.Prepare(&url);
    webClient.HttpGet();
  	ESP_LOGW(LOGTAG, "END https://httpbin.org/headers");




  	ESP_LOGW(LOGTAG, "POST TEST: https://httpbin.org/post");
    url.Parse("http://httpbin.org/post");
    if (!webClient.Prepare(&url)) {
    	ESP_LOGE(LOGTAG, "Error in Preparing POST()")
    }

    std::string post = "{ example: \"data\" }";
  	ESP_LOGW(LOGTAG, "BEFORE POSTING");
    if (!webClient.HttpPost(post)) {
      	ESP_LOGE(LOGTAG, "Error in executing POST()")
    }
  	ESP_LOGW(LOGTAG, "AFTER POSTING"); */


/*	ESP_LOGW(LOGTAG, "#####Starting Firmware Update Task ....");

  	Ota ota;
    if(ota.UpdateFirmware("http://surpro4:9999/getfirmware")) {
	  	ESP_LOGI(LOGTAG, "#####AFTER OTA STUFF---- RESTARTING IN 2 SEC");
		vTaskDelay(2*1000 / portTICK_PERIOD_MS);
		esp_restart();
    } else {
    	//TODO add ota.GetErrorInfo() to inform end-user of problem
	  	ESP_LOGE(LOGTAG, "#####OTA update failed!");
    }*/

}

void Esp32Gong::TaskWebServer() {
	webServer.Start();
}

void Esp32Gong::TaskDnsServer() {
	dnsServer.start();
}

void Esp32Gong::TaskResetButton() {
	int level = 0;
	int ticks = 0;
	int tempticks = 0;



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
		tempticks++;
		if (tempticks > 1*60) {
			float t = esp32_temperature();
			ESP_LOGI(LOGTAG, "Temperature %3.1f", t);
			tempticks = 0;
		}

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

