/*
 * Ota.cpp
 *
 *  Created on: 11.04.2017
 *      Author: bernd
 */
#include "Ota.h"

#include "sdkconfig.h"
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_partition.h>

#include <nvs.h>
#include <nvs_flash.h>

#include "WebClient.h"

#define BUFFSIZE 1024
#define TEXT_BUFFSIZE 1024

static const char* LOGTAG = "ota";

Ota::Ota() {

}

Ota::~Ota() {
	// TODO Auto-generated destructor stub
}


static void __attribute__((noreturn)) task_fatal_error()
{
    ESP_LOGE(LOGTAG, "Exiting task due to fatal error...");
    (void)vTaskDelete(NULL);

    while (1) {
        ;
    }
}


bool Ota::OnReceiveBegin() {
    ESP_LOGI(LOGTAG, "Starting OTA example...");

	esp_err_t err;
    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    ESP_LOGI(LOGTAG, "Running partition type %d subtype %d (offset 0x%08x)",
             configured->type, configured->subtype, configured->address);

    mpUpdatePartition = esp_ota_get_next_update_partition(NULL);
    if (mpUpdatePartition == NULL) {
        ESP_LOGE(LOGTAG, "could not get next update partition");
    	return false;
    }

    ESP_LOGI(LOGTAG, "Writing to partition subtype %d at offset 0x%x",
             mpUpdatePartition->subtype, mpUpdatePartition->address);


    err = esp_ota_begin(mpUpdatePartition, OTA_SIZE_UNKNOWN, &mOtaHandle);
    if (err != ESP_OK) {
        ESP_LOGE(LOGTAG, "esp_ota_begin failed, error=%d", err);
        task_fatal_error();
        return false;
    }
    ESP_LOGI(LOGTAG, "esp_ota_begin succeeded");
    return true;
}

bool Ota::OnReceiveData(char* buf, int len) {
	esp_err_t err;
    err = esp_ota_write( mOtaHandle, (const void *)buf, len);
    if (err == ESP_ERR_INVALID_SIZE) {
    	ESP_LOGE(LOGTAG, "Error partition too small for firmware data: %d", muDataLength + len );
    } else if (err != ESP_OK) {
    	ESP_LOGE(LOGTAG, "Error writing data: %d", err);
    	return false;
    }
    muDataLength += len;
    ESP_LOGI(LOGTAG, "Have written image length %d, total %d", len, muDataLength);
    return err == ESP_OK;
}

void Ota::OnReceiveEnd() {
    ESP_LOGI(LOGTAG, "Total Write binary data length : %u", muDataLength);
    //ESP_LOGI(LOGTAG, "DATA: %s", dummy.c_str());

    esp_err_t err;

    if (esp_ota_end(mOtaHandle) != ESP_OK) {
        ESP_LOGE(LOGTAG, "esp_ota_end failed!");
        task_fatal_error();
    }
    err = esp_ota_set_boot_partition(mpUpdatePartition);
    if (err != ESP_OK) {
        ESP_LOGE(LOGTAG, "esp_ota_set_boot_partition failed! err=0x%x", err);
        task_fatal_error();
    }
    ESP_LOGI(LOGTAG, "Prepare to restart system!");
    mbEndSuccess = true;
    //esp_restart();
}



bool Ota::UpdateFirmware(std::string sUrl)
{
	Url url;
	url.Parse(sUrl);

	ESP_LOGI(LOGTAG, "Retrieve firmware from: %s", url.GetUrl().c_str());
	mWebClient.Prepare(&url);
	mWebClient.SetDownloadHandler(this);

    if (!mWebClient.HttpGet()) {
      	ESP_LOGE(LOGTAG, "Error in HttpExecute()")
      			return false;
    }

	ESP_LOGI(LOGTAG, "UpdateFirmware finished. downloaded %u bytes, success %s" , muDataLength, mbEndSuccess ? "yeah!": "uhhh!");

    return mbEndSuccess;

}

void task_function_firmwareupdate(void* user_data) {
	ESP_LOGW(LOGTAG, "Starting Firmware Update Task ....");

  	Ota ota;
    if(ota.UpdateFirmware("http://surpro4:9999/getfirmware")) {
	  	ESP_LOGI(LOGTAG, "AFTER OTA STUFF---- RESTARTING IN 2 SEC");
		vTaskDelay(2*1000 / portTICK_PERIOD_MS);
		esp_restart();
    } else {
    	//TODO add ota.GetErrorInfo() to inform end-user of problem
	  	ESP_LOGE(LOGTAG, "OTA update failed!");
    }

}



void Ota::StartUpdateFirmwareTask() {
	xTaskCreate(&task_function_firmwareupdate, "firmwareupdate", 4096, NULL, 5, NULL);
}

