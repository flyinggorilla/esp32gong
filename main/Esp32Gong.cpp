#include "Esp32Gong.h"

#include "sdkconfig.h"
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <stdio.h>
#include <string>
#include "math.h"
#include "string.h"

#include "Config.h"
#include "I2SPlayer.h"
#include "Esp32GongWebServer.h"
#include "Wifi.h"
#include "Storage.h"
#include "Url.h"
#include "WebClient.h"
#include "Ota.h"
#include "PingWatchdog.h"

#define ONBOARDLED_GPIO GPIO_NUM_5 // GPIO5 on Sparkfun ESP32 Thing
#define LOGTAG "main"

I2SPlayer musicPlayer;
Esp32GongWebServer webServer;
Esp32Gong esp32gong;
Storage storage;
Wifi wifi;

// Wav* wav = NULL;
Esp32Gong::Esp32Gong()
{
	//	wav = NULL;
}

Esp32Gong::~Esp32Gong()
{
	//	delete wav;
}

extern "C"
{
	void app_main();
}

void app_main()
{
	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(esp_netif_init());
	esp32gong.Start();
}

//===========================================

void task_function_webserver(void *pvParameter)
{
	((Esp32Gong *)pvParameter)->TaskWebServer();
	vTaskDelete(NULL);
}

void task_function_resetbutton(void *pvParameter)
{
	((Esp32Gong *)pvParameter)->TaskResetButton();
	vTaskDelete(NULL);
}

/*
void task_function_dnsserver(void *pvParameter) {
	((Esp32Gong*) pvParameter)->TaskDnsServer();
	vTaskDelete(NULL);
}
*/

void task_test_webclient(void *pvParameter)
{
	((Esp32Gong *)pvParameter)->TaskTestWebClient();
	vTaskDelete(NULL);
}

void task_function_restart(void *user_data)
{
	ESP_LOGI(LOGTAG, "Restarting in 2secs....");
	vTaskDelay(*((int *)user_data) * 1000 / portTICK_PERIOD_MS);
	esp_restart();
}

void Esp32Gong::Restart(int seconds)
{
	xTaskCreate(&task_function_restart, "restartTask", 2048, &seconds, 5, NULL);
}

//----------------------------------------------------------------------------------------

void Esp32Gong::Start()
{

	ESP_LOGI(LOGTAG, "Welcome to Bernd's ESP32 Gong");
	ESP_LOGI(LOGTAG, "ESP-IDF version %s", esp_get_idf_version());
	ESP_LOGI(LOGTAG, "Firmware version %s", FIRMWARE_VERSION);

	musicPlayer.init();

	mConfig.Read();

	storage.Mount();

	#ifdef WIFIMODEBUTTON_GPIO
		mbButtonPressed = !gpio_get_level(GPIO_NUM_0);

		gpio_pad_select_gpio(GPIO_NUM_0); // TODO *********************
		gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
		gpio_set_pull_mode(GPIO_NUM_0, GPIO_PULLUP_ONLY);

	#else
		gpio_pad_select_gpio(GPIO_NUM_0); // TODO *********************
		gpio_set_direction(GPIO_NUM_0, GPIO_MODE_OUTPUT);
		mbButtonPressed = false;
	#endif

	gpio_pad_select_gpio((gpio_num_t)ONBOARDLED_GPIO);
	gpio_set_direction((gpio_num_t)ONBOARDLED_GPIO, (gpio_mode_t)GPIO_MODE_OUTPUT);

	xTaskCreate(&task_function_webserver, "Task_WebServer", 8192, this, 5, NULL);
	xTaskCreate(&task_function_resetbutton, "Task_ResetButton", 2048, this, 5, NULL);

	//ESP_LOGI(LOGTAG, "CONFIG HOSTNAME: %s", mConfig.msHostname.c_str() == NULL ? "NULL" : mConfig.msHostname.c_str());
	//ESP_LOGI(LOGTAG, "APMode from config %d", mConfig.mbAPMode);

	if (mConfig.mbAPMode)
	{
		if (mConfig.muLastSTAIpAddress)
		{
			char sBuf[16];
			sprintf(sBuf, "%d.%d.%d.%d", IP2STR((ip4_addr *)&mConfig.muLastSTAIpAddress));
			ESP_LOGD(LOGTAG, "Last IP when connected to AP: %d : %s", mConfig.muLastSTAIpAddress, sBuf);
		}

		ESP_LOGI(LOGTAG, "Access Point mode. Please connect SSID 'esp32gong' at IP '192.168.4.1'");
		wifi.StartAPMode(mConfig.msAPSsid, mConfig.msAPPass, mConfig.msHostname);
		// start DNS server to always redirect any domain to 192.168.4.1
		//		xTaskCreate(&task_function_dnsserver, "Task_DnsServer", 8192, this, 5, NULL);
	}
	else
	{
		if (mConfig.msSTAENTUser.length())
			wifi.StartSTAModeEnterprise(mConfig.msSTASsid, mConfig.msSTAENTUser, mConfig.msSTAPass, mConfig.msSTAENTCA, mConfig.msHostname);
		else
			wifi.StartSTAMode(mConfig.msSTASsid, mConfig.msSTAPass, mConfig.msHostname);
		//wifi.StartMDNS();

		int wifiConnectTries = 0;
		while (true)
		{
			wifiConnectTries++;
			if (wifi.IsConnected()) {
				break;
			} else if (wifiConnectTries > 100) {
				ESP_LOGW(LOGTAG, "Could not get an IP address after trying for a while. Restarting.");
				esp_restart();
			}
			vTaskDelay(1000 / portTICK_PERIOD_MS);
		}
		PingWatchdog *ping = new PingWatchdog();
		ping->Start(wifi.GetGWAddress());
	}

	//while(!wifi.IsConnected()) {
	//	vTaskDelay(100/portTICK_PERIOD_MS);
	//}

	//xTaskCreate(&task_test_webclient, "Task_TestWebClient", 8192, this, 5, NULL);
	//ESP_LOGI(LOGTAG, "********************** OTA MAIN VERSION *********************");
	//Ota ota;
	//ota.UpdateFirmware("https://surpro4:9999/getfirmware");
	//ESP_LOGI(LOGTAG, "********************** OTA THREAD VERSION PINNED TO CORE *********************");
	//Ota::StartUpdateFirmwareTask();

	// keep this thread alive, otherwise I2S will die
	ESP_LOGI(LOGTAG, "---------- Startup completed -----------");
}

void Esp32Gong::TaskTestWebClient()
{
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

void Esp32Gong::TaskWebServer()
{
	while (true)
	{
		if (wifi.IsConnected())
		{
			ESP_LOGI("Esp32Gong", "starting Webserver");
			webServer.StartWebServer();
		}
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void Esp32Gong::TaskDnsServer()
{
	//	dnsServer.start();
}

void Esp32Gong::TaskResetButton()
{
	int level = 0;
	int ticks = 0;

	while (true)
	{
		if (wifi.IsConnected())
		{
			gpio_set_level((gpio_num_t)ONBOARDLED_GPIO, (gpio_mode_t)level);
			level = 0;
			ticks = 0;
		}
		else
		{
			int minticks = 0;
			if (mConfig.mbAPMode)
				minticks = 1; // blink slow in APmode

			if (ticks > minticks)
			{ // blink fast when no wifi connection
				level = !level;
				ticks = 0;
			}
		}
		ticks++;

		gpio_set_level((gpio_num_t)ONBOARDLED_GPIO, (gpio_mode_t)level);

		// DONT USE ONBOARD BOOT BUTTON GPIO0, AS USB POWER DEVICES MAY INADVERTADLY TRIGGER AP MODE 
		//   USE an extra button switch and a GPIO that does not conflict with boot and serial modes

		#ifdef WIFIMODEBUTTON_GPIO
			if (!gpio_get_level(WIFIMODEBUTTON_GPIO))
			{
				if (!mbButtonPressed)
				{
					ESP_LOGI(LOGTAG, "Factory settings button pressed... rebooting into Access Point mode.");
					mConfig.ToggleAPMode();
					mConfig.Write();
					esp_restart();
				}
			}
			else
			{
				mbButtonPressed = false;
			}
		#endif

		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}

//-----------------------------------------------------------------------------------------
