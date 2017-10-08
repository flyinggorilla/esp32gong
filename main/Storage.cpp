/*
 * Storage.cpp
 *
 */
#include "Storage.h"

#include "sdkconfig.h"
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
#include <esp_system.h>

#include <nvs.h>
#include <nvs_flash.h>

#include "String.h"
#include "WebClient.h"

static const char* LOGTAG = "Storage";


volatile int Storage::miProgress = STORAGE_PROGRESS_NOTYETSTARTED;
volatile unsigned int Storage::muTimestamp = 0;
int Storage::GetProgress() { return miProgress; }
unsigned int Storage::GetTimestamp() { return muTimestamp; }


Storage::Storage() {
    miProgress = STORAGE_PROGRESS_NOTYETSTARTED;
    muTimestamp = esp_log_early_timestamp();
}

Storage::~Storage() {
	// TODO Auto-generated destructor stub
}


bool Storage::InternalOnRecvBegin(bool isContentLength, unsigned int contentLength){
    

    ESP_LOGI(LOGTAG, "InternalOnRecvBegin(%i)", isContentLength ? contentLength : -1);

    if (isContentLength) {
        muContentLength = contentLength;
    } else {
        //@TODO fix size estimation (also in Storage.cpp)
        // http://esp-idf.readthedocs.io/en/latest/api-reference/storage/spi_flash.html
        muContentLength = 1536*1024; // we use Storage partition size when we dont have exact firmware size
    }
    miProgress = 0;
    muActualDataLength = 0;
    
    if (!Open(mFilename)) {
        ESP_LOGE(LOGTAG, "could not open test.wav");
        miProgress = STORAGE_PROGRESS_FLASHERROR;
        return false;
    }
    return true;
}

bool Storage::OnReceiveBegin(unsigned short int httpStatusCode, bool isContentLength, unsigned int contentLength) {
    ESP_LOGD(LOGTAG, "OnReceiveBegin(%u, %u)", httpStatusCode, contentLength);

    if (httpStatusCode != 200)
        return false;
    return InternalOnRecvBegin(isContentLength, contentLength);
}

bool Storage::OnReceiveBegin(String& sFilename, unsigned int contentLength){
    ESP_LOGD(LOGTAG, "OnReceiveBegin(%s, %u)", sFilename.c_str(), contentLength);
    mFilename = sFilename;
    return InternalOnRecvBegin(true, contentLength);
}

bool Storage::OnReceiveData(char* buf, int len) {
    ESP_LOGD(LOGTAG, "OnReceiveData(%d)", len);
    String sbuf;
    sbuf.concat(buf, len);
    //ESP_LOGD(LOGTAG, ">>DATA>>%s<<DATA<<", sbuf.c_str());


    if (!Write(buf, len)) {
    	ESP_LOGE(LOGTAG, "Error writing - partition too small or full? size %d", muActualDataLength + len );
        miProgress = STORAGE_PROGRESS_FLASHERROR;
    	return false;
    } 
    
    muActualDataLength += len;
    miProgress = 100 * muActualDataLength / muContentLength;
    ESP_LOGD(LOGTAG, "Have written image length %d, total %d", len, muActualDataLength);
    return true;
}

bool Storage::OnReceiveEnd() {
    ESP_LOGI(LOGTAG, "Total Write binary data length : %u", muActualDataLength);
    //ESP_LOGI(LOGTAG, "DATA: %s", dummy.c_str());

    /*if (!Close()) {
        ESP_LOGE(LOGTAG, "Closing test.wav failed!");
        miProgress = STORAGE_PROGRESS_FLASHERROR;
        //task_fatal_error();
        return false;
    }*/

    Close();

    ESP_LOGD(LOGTAG, "File Writtenn successfully!");
    miProgress = STORAGE_PROGRESS_FINISHEDSUCCESS;
    return true;
}



bool Storage::DownloadFile(String& sUrl)
{
	Url url;
	url.Parse(sUrl);

	ESP_LOGI(LOGTAG, "Retrieve file from: %s", url.GetUrl().c_str());
    
    WebClient webClient;
    webClient.Prepare(&url);
	webClient.SetDownloadHandler(this);

    unsigned short statuscode = webClient.HttpGet();
    if (statuscode != 200) {
        if (miProgress == STORAGE_PROGRESS_NOTYETSTARTED || miProgress >= 0) {
            miProgress = STORAGE_PROGRESS_CONNECTIONERROR;
        }
      	ESP_LOGE(LOGTAG, "Storage update failed - error %u", statuscode)
        // esp_reboot();
      	return false;
    }

	ESP_LOGI(LOGTAG, "DownloadFile finished successfully. downloaded %u bytes" , muActualDataLength);

    return true;

}


/*
void task_function_firmwareupdate(void* user_data) {
	ESP_LOGW(LOGTAG, "Starting Firmware Update Task ....");

    Storage ota;
    if(ota.UpdateFirmware((const char*)user_data)) { //url
      	ESP_LOGI(LOGTAG, "Firmware updated. Rebooting now......");
    } else {
	  	ESP_LOGE(LOGTAG, "OTA update failed!");
    }
  
    // wait 10 seconds before rebooting to make sure client gets success info
    vTaskDelay(10*1000 / portTICK_PERIOD_MS);
	esp_restart();
}



void Storage::StartUpdateFirmwareTask(const char* url) {
    miProgress = 0;
	//xTaskCreate(&task_function_firmwareupdate, "firmwareupdate", 8192, NULL, 5, NULL);
    // Pin firmware update task to core 0 --- otherwise we get weird crashes
   	xTaskCreatePinnedToCore(&task_function_firmwareupdate, "firmwareupdate", 8192, (void*)url, 6, NULL, 0);
}*/

