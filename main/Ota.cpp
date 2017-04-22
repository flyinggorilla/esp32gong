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

	/*esp_err_t err;
    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    assert(configured == running); // fresh from reset, should be running from configured boot partition
    ESP_LOGI(LOGTAG, "Running partition type %d subtype %d (offset 0x%08x)",
             configured->type, configured->subtype, configured->address);

    update_partition = esp_ota_get_next_update_partition(NULL);

    ESP_LOGI(LOGTAG, "Writing to partition subtype %d at offset 0x%x",
             update_partition->subtype, update_partition->address);
    assert(update_partition != NULL);

    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(LOGTAG, "esp_ota_begin failed, error=%d", err);
        task_fatal_error();
    }
    ESP_LOGI(LOGTAG, "esp_ota_begin succeeded"); */
    dummy = "";
    return true;
}

bool Ota::OnReceiveData(char* buf, int len) {
	//esp_err_t err;
    //err = esp_ota_write( update_handle, (const void *)buf, len);
    ESP_LOGI(LOGTAG, "Have written image length %d", len);
    muDataLength += len;
    //return err == ESP_OK;

    //dummy.append(buf, 0, len);
    //ESP_LOGI(LOGTAG, "DATA: %s", dummy.c_str());


    return true;
}

void Ota::OnReceiveEnd() {
    ESP_LOGI(LOGTAG, "Total Write binary data length : %u", muDataLength);
    //ESP_LOGI(LOGTAG, "DATA: %s", dummy.c_str());
/*
    esp_err_t err;

    if (esp_ota_end(update_handle) != ESP_OK) {
        ESP_LOGE(LOGTAG, "esp_ota_end failed!");
        task_fatal_error();
    }
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(LOGTAG, "esp_ota_set_boot_partition failed! err=0x%x", err);
        task_fatal_error();
    }
    ESP_LOGI(LOGTAG, "Prepare to restart system!");
    esp_restart(); */
}



bool Ota::UpdateFirmware(std::string sUrl)
{
	ESP_LOGI(LOGTAG, "OTA not yet fully implemented --- testing DownloadHandler right now");
	Url url;
	url.Parse(sUrl);

	ESP_LOGI(LOGTAG, "Retrieve firmware from: %s", url.GetUrl().c_str());
	mWebClient.Prepare(&url);

    if (!mWebClient.HttpExecute(this)) {
      	ESP_LOGE(LOGTAG, "Error in HttpExecute()")
      			return false;
    }
    return true;

}



