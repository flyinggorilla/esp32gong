/*
 * Ota.hpp
 *
 *  Created on: 11.04.2017
 *      Author: bernd
 */

#ifndef MAIN_OTA_H_
#define MAIN_OTA_H_

#include <esp_ota_ops.h>
#include <string>
#include "DownloadHandler.h"
#include "WebClient.h"




class Ota : public DownloadHandler {
public:
	/*static void StartUpdateFirmwareTask();
	static int  smErrorCode; //TODO this should provide "feedback" from the static class*/

public:
	Ota();
	virtual ~Ota();
	bool UpdateFirmware(std::string url);

public:
	bool OnReceiveBegin(unsigned short int httpStatusCode, bool isContentLength, unsigned int contentLength);
	void OnReceiveEnd();
	bool OnReceiveData(char* buf, int len); // override DownloadHandler virtual method

private:
	WebClient mWebClient;
    esp_ota_handle_t mOtaHandle = 0 ;
    const esp_partition_t *mpUpdatePartition = NULL;
    unsigned int muDataLength = 0;
    bool mbUpdateFailed = true;
};

#endif /* MAIN_OTA_H_ */
