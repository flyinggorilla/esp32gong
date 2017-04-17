/*
 * WebClient.cpp
 *
 *  Created on: 09.04.2017
 *      Author: bernd
 */

#include "WebClient.hpp"

#include <esp_log.h>
#include "sdkconfig.h"
#include <string.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <netdb.h>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "UriParser.hpp"

static const char LOGTAG[] = "WebClient";

WebClient::WebClient() {

}

WebClient::~WebClient() {

}

void WebClient::ConnectEvent(int errorcode) {

	if (!downloadHandler) {
		return;
	}
	downloadHandler->OnReceiveBegin();
}

void WebClient::ReceiveEvent(char* data, size_t len) {
	char sbuf[128];
	memset(sbuf, 0, sizeof(sbuf));
	int copylen = len < sizeof(sbuf) ? len : sizeof(sbuf) - 1;
	memcpy(sbuf, data, copylen);
	ESP_LOGI(LOGTAG, "data: %s", sbuf);

	if (!downloadHandler) {
		return;
	}

	downloadHandler->OnReceiveData(data, len); //OR received???

}

void WebClient::CloseEvent() {
	if (exit_flag == 0) {
		ESP_LOGI(LOGTAG, "server closed connection");
		exit_flag = 1;
	}
	if (!downloadHandler) {
		return;
	}
	downloadHandler->OnReceiveEnd();
}

void WebClient::ReplyEvent() {

}

bool WebClient::request(const char* url, DownloadHandler* downloadHandler) {

	ESP_LOGI(LOGTAG, "Requesting %s", url);
	this->downloadHandler = downloadHandler;

	return HttpGetRequest(url);

	//ESP_LOGE(LOGTAG, "Error connecting - HTTP status code %i", status);
	//return false;
}

bool WebClient::HttpGetRequest(const char* url) {
	struct addrinfo hints;
    memset( &hints, 0, sizeof( hints ) );
    hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;


	struct addrinfo *res;
	struct in_addr *addr;
	int s, r;
	char recv_buf[64];

	ESP_LOGI(LOGTAG, "HttpGetRequest: %s", url);

	UriParser uri;
	if (!uri.parseUrl(url)) {
		ESP_LOGE(LOGTAG, "Invalid URI %s", url);
		return false;
	}
	ESP_LOGI(LOGTAG, "after url parsing: hostname=<%s> path=<%s>", uri.GetHost(), uri.GetPath());

	char service[6];
	sprintf(service, "%i", uri.GetPort());
	int err = getaddrinfo(uri.GetHost(), "80", &hints, &res);

	if (err != 0 || res == NULL) {
		ESP_LOGE(LOGTAG, "DNS lookup failed err=%d res=%p", err, res);
		return false;
	}

	/* Code to print the resolved IP.

	 Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
	addr = &((struct sockaddr_in *) res->ai_addr)->sin_addr;
	ESP_LOGI(LOGTAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

	s = socket(res->ai_family, res->ai_socktype, 0);
	if (s < 0) {
		ESP_LOGE(LOGTAG, "... Failed to allocate socket.");
		freeaddrinfo(res);
		return false;
	}
	ESP_LOGI(LOGTAG, "... allocated socket\r\n");

	if (connect(s, res->ai_addr, res->ai_addrlen) != 0) {
		ESP_LOGE(LOGTAG, "... socket connect failed errno=%d", errno);
		close(s);
		freeaddrinfo(res);
		return false;
	}

	ESP_LOGI(LOGTAG, "... connected");
	freeaddrinfo(res);

	std::string request;
	request = "GET ";
	request += "/";
	request += uri.GetPath();
	request += uri.GetQuery();
	request += " HTTP/1.0\r\nHost: ";
	request += uri.GetHost();
	request += "\r\nUser-Agent: esp32webclient/1.0 esp32\r\n\r\n";
	ESP_LOGI(LOGTAG, "request: %s", request.c_str());
	if (write(s, request.c_str(), request.length()) < 0) {
		ESP_LOGE(LOGTAG, "... socket send failed");
		close(s);
		return false;
	}
	ESP_LOGI(LOGTAG, "... socket send success");

	/* Read HTTP response */
	int bytes = 0;
	std::string data;
	do {
		bzero(recv_buf, sizeof(recv_buf));
		r = read(s, recv_buf, sizeof(recv_buf) - 1);
		if (r > 0) {
			bytes += r;
			if (bytes < 1024) {
				data.append(recv_buf, 0, r);
			}
		}
	} while (r > 0);
	ESP_LOGI(LOGTAG, "data %i bytes: %s", bytes, data.c_str());

	ESP_LOGI(LOGTAG, "... done reading from socket. Last read return=%d errno=%d\r\n", r, errno);
	close(s);

	return true;
}

