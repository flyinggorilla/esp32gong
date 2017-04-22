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
	Ota();
	virtual ~Ota();
	bool UpdateFirmware(std::string url);

public:
	bool OnReceiveBegin();
	void OnReceiveEnd();
	bool OnReceiveData(char* buf, int len); // override DownloadHandler virtual method

private:
	WebClient mWebClient;
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;
    unsigned int muDataLength = 0;

    std::string dummy; //TODO REMOVE

};

#endif /* MAIN_OTA_H_ */
