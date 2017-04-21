#include "WebClient.h"

#include "Url.hpp"
#include <esp_log.h>
#include "sdkconfig.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <list>
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <netdb.h>

#include "HttpResponseParser.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

static const char LOGTAG[] = "WebClient";


WebClient::WebClient() {

}

WebClient::~WebClient() {

}


bool WebClient::HttpPrepareGet(Url* pUrl, DownloadHandler* pOptionalDownloadHandler) {
	mpDownloadHandler = pOptionalDownloadHandler;
	mlRequestHeaders.clear();
	if (!pUrl)
		return false;
	mpUrl = pUrl;
	return true;
}

bool WebClient::HttpAddHeader(std::string& sHeader) {
	mlRequestHeaders.push_back(sHeader); //TODO IS THIS COPYING THE STRING?? YES = SAFE NO = NEED TO COPY BEFOREHAND
	return true;
}

bool WebClient::HttpExecute() {
	if (!mpUrl)
		return false;

	if (mpUrl->GetHost().empty()) {
		return false;
	}

	struct addrinfo *res;
	char service[6];
	sprintf(service, "%i", mpUrl->GetPort());
	struct addrinfo hints;
    memset( &hints, 0, sizeof( hints ) );
    hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	int err = getaddrinfo(mpUrl->GetHost().c_str(), service, &hints, &res);

	if (err != 0 || res == NULL) {
		ESP_LOGE(LOGTAG, "DNS lookup failed err=%d res=%p", err, res);
		return false;
	}

	// Code to print the resolved IP.
	// Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code
	struct in_addr *addr = &((struct sockaddr_in *) res->ai_addr)->sin_addr;
	ESP_LOGI(LOGTAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

	// Socket
	int socket = socket(res->ai_family, res->ai_socktype, 0);
	if (socket < 0) {
		ESP_LOGE(LOGTAG, "... Failed to allocate socket.");
		freeaddrinfo(res);
		return false;
	}
	ESP_LOGI(LOGTAG, "... allocated socket\r\n");

	// CONNECT
	if (connect(socket, res->ai_addr, res->ai_addrlen) != 0) {
		ESP_LOGE(LOGTAG, "... socket connect failed errno=%d", errno);
		close(socket);
		freeaddrinfo(res);
		return false;
	}
	ESP_LOGI(LOGTAG, "... connected");
	freeaddrinfo(res);

	// Build HTTP Request
	std::string sRequest;

	sRequest.reserve(512);
	sRequest = "GET ";
	sRequest += "/";
	sRequest += mpUrl->GetPath();
	sRequest += mpUrl->GetQuery();
	sRequest += " HTTP/1.0\r\nHost: ";
	sRequest += mpUrl->GetHost();
	sRequest += "\r\n";
	for (std::list<std::string>::iterator it = mlRequestHeaders.begin(); it != mlRequestHeaders.end(); ++it) {
		sRequest += *it; //TODO *it or it????
		sRequest += "\r\n";
	}
	sRequest+= "User-Agent: esp32webclient/1.0 esp32\r\n\r\n";

	// send HTTP request
	ESP_LOGI(LOGTAG, "sRequest: %s", sRequest.c_str());
	if (write(socket, sRequest.c_str(), sRequest.length()) < 0) {
		ESP_LOGE(LOGTAG, "... socket send failed");
		close(socket);
		return false;
	}
	sRequest.clear(); // free memory
	ESP_LOGI(LOGTAG, "... socket send success");

	// Read HTTP response
	HttpResponseParser httpParser;
	httpParser.Init(mpDownloadHandler);

	char recv_buf[256];
	while (!httpParser.ResponseFinished()) {
		size_t sizeRead = read(socket, recv_buf, sizeof(recv_buf) - 1);
		if (sizeRead <= 0) {
			ESP_LOGE(LOGTAG, "Connection closed during parsing");
			close(socket);
			break;
		}
		if (!httpParser.ParseResponse(recv_buf, sizeRead)) {
			ESP_LOGE(LOGTAG, "HTTP Parsing error: %d", httpParser.GetError());
			close(socket);
			return false;
		}
	}

	ESP_LOGI(LOGTAG, "data %i bytes: %s", httpParser.GetContentLength(), httpParser.GetBody().c_str());

	close(socket);

	return true;
}



