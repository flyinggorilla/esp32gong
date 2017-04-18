/*
 * Ota.hpp
 *
 *  Created on: 11.04.2017
 *      Author: bernd
 */

#ifndef MAIN_OTA_HPP_
#define MAIN_OTA_HPP_

#include "WebClient.hpp"
#include "DownloadHandler.hpp"
#include <esp_ota_ops.h>
#include <string>

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
	WebClient webClient;


    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;

};

#endif /* MAIN_OTA_HPP_ */
