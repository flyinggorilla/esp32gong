#include "WebServer.h"

#include "sdkconfig.h"
#include <lwip/sockets.h>
#include <esp_log.h>
#include <string.h>
#include <esp_event.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <string>
#include "time.h"
#include "cJSON.h"

#include "Config.h"
#include "DynamicRequestHandler.h"
#include "Esp32Gong.h"
#include "fontwoff.h"
#include "fontttf.h"
#include "fontsvg.h"
#include "fonteot.h"
#include "HttpRequestParser.h"
#include "HttpResponse.h"
#include "indexhtml.h"
#include "Ota.h"
#include "SpiffsFileSystem.h"
#include "UrlParser.h"

#include "Wifi.h"

extern Esp32Gong esp32gong;

static char logtag[] = "WebServer";

struct TServerSocketPair {
	WebServer* pServer;
	int socket;
};

void request_handler_function(void *pvParameter);

//------------------------------------------------------------------

WebServer::WebServer() {

}

WebServer::~WebServer() {
}

bool WebServer::Start(__uint16_t port) {
	struct sockaddr_in clientAddress;
	struct sockaddr_in serverAddress;

	// Create a socket that we will listen upon.
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		ESP_LOGE(logtag, "socket: %d %s", sock, strerror(errno));
		return false;
	}

	// Bind our server socket to a port.
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(port);
	int rc = bind(sock, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
	if (rc < 0) {
		ESP_LOGE(logtag, "bind: %d %s", rc, strerror(errno));
		return false;
	}

	// Flag the socket as listening for new connections.
	rc = listen(sock, 5);
	if (rc < 0) {
		ESP_LOGE(logtag, "listen: %d %s", rc, strerror(errno));
		return false;
	}
	ESP_LOGI(logtag, "started listening");

	while (1) {
		// Listen for a new client connection.
		socklen_t clientAddressLength = sizeof(clientAddress);
		int clientSock = accept(sock, (struct sockaddr *) &clientAddress, &clientAddressLength);
		if (clientSock < 0) {
			ESP_LOGE(logtag, "accept: %d %s", clientSock, strerror(errno));
			return false;
		}
		ESP_LOGD(logtag, "new connection");

		TServerSocketPair* pServerSocketPair = (TServerSocketPair*) malloc(sizeof(TServerSocketPair));
		pServerSocketPair->pServer = this;
		pServerSocketPair->socket = clientSock;
		xTaskCreate(&request_handler_function, "WebSocketHandler", 4096, pServerSocketPair, 5, NULL);
	}

	//vTaskDelete(NULL);
}

void request_handler_function(void *pvParameter) {
	TServerSocketPair* serverSocket = (TServerSocketPair*) pvParameter;
	serverSocket->pServer->WebRequestHandler(serverSocket->socket);
	vTaskDelete(NULL);
	delete serverSocket;
}

void WebServer::WebRequestHandler(int socket) {

	// We now have a new client ...
	int total = 1024;
	char *data = (char*) malloc(total);
	HttpRequestParser httpParser;
	HttpResponse httpResponse;
	DynamicRequestHandler requestHandler;

	while (1) {
		httpParser.Init();

		while (1) {
			ssize_t sizeRead = recv(socket, data, total, 0);
			if (sizeRead <= 0) {
				ESP_LOGE(logtag, "Connection closed during parsing");
				free(data);
				close(socket);
				return;
			}
			if (!httpParser.ParseRequest(data, sizeRead)) {
				ESP_LOGE(logtag, "HTTP Parsing error: %d", httpParser.GetError());
				free(data);
				close(socket);
				return;
			}
			if (httpParser.RequestFinished()) {
				break;
			}
		}
		// request parsed

		if (!httpParser.GetUrl().compare("/") || !httpParser.GetUrl().compare("/index.html")) {
			httpResponse.Init(200, httpParser.IsHttp11(), httpParser.IsConnectionClose());
			httpResponse.AddHeader("Content-Type: text/html");
			httpResponse.AddHeader("Content-Encoding: gzip");
			if (!httpResponse.Send(socket, indexhtml_h, sizeof(indexhtml_h)))
				break;
		} else if (!httpParser.GetUrl().compare("/fonts/material-design-icons.woff")) {
			httpResponse.Init(200, httpParser.IsHttp11(), httpParser.IsConnectionClose());
			httpResponse.AddHeader("Content-type: application/octet-stream");
			if (!httpResponse.Send(socket, fontwoff_h, sizeof(fontwoff_h)))
				break;
		} else if (!httpParser.GetUrl().compare("/fonts/material-design-icons.ttf")) {
			httpResponse.Init(200, httpParser.IsHttp11(), httpParser.IsConnectionClose());
			httpResponse.AddHeader("Content-type: application/octet-stream");
			if (!httpResponse.Send(socket, fontttf_h, sizeof(fontttf_h)))
				break;
		} else if (!httpParser.GetUrl().compare("/fonts/material-design-icons.eot")) {
			httpResponse.Init(200, httpParser.IsHttp11(), httpParser.IsConnectionClose());
			httpResponse.AddHeader("Content-type: application/octet-stream");
			if (!httpResponse.Send(socket, fonteot_h, sizeof(fonteot_h)))
				break;
		} else if (!httpParser.GetUrl().compare("/fonts/material-design-icons.svg")) {
			httpResponse.Init(200, httpParser.IsHttp11(), httpParser.IsConnectionClose());
			httpResponse.AddHeader("Content-type: application/octet-stream");
			if (!httpResponse.Send(socket, fontsvg_h, sizeof(fontsvg_h)))
				break;
		} else if (!httpParser.GetUrl().compare("/api")) {
			std::string sBody;
			__uint8_t retCode = requestHandler.HandleApiRequest(httpParser.GetParams(), sBody);
			httpResponse.Init(retCode, httpParser.IsHttp11(), httpParser.IsConnectionClose());
			httpResponse.AddHeader("cache-control: private, max-age=0, no-cache, no-store");
			if (!httpResponse.Send(socket, sBody.data(), sBody.size()))
				break;
		} else if (!httpParser.GetUrl().compare("/info")) {
			std::string sBody;
			__uint8_t retCode = requestHandler.HandleInfoRequest(httpParser.GetParams(), sBody);
			httpResponse.Init(retCode, httpParser.IsHttp11(), httpParser.IsConnectionClose());
			httpResponse.AddHeader("cache-control: private, max-age=0, no-cache, no-store");
			if (!httpResponse.Send(socket, sBody.data(), sBody.size()))
				break;
		} else if (!httpParser.GetUrl().compare("/config")) {
			std::string sBody;
			__uint8_t retCode = requestHandler.HandleConfigRequest(httpParser.GetParams(), sBody);
			httpResponse.Init(retCode, httpParser.IsHttp11(), httpParser.IsConnectionClose());
			char sBuf[256];
			sprintf(sBuf, "Content-type: text/html\r\nRefresh:10; url=http://%s",
					esp32gong.GetConfig().msHostname.c_str());
			httpResponse.AddHeader(sBuf);
			if (!httpResponse.Send(socket, sBody.data(), sBody.size()))
				break;
		}

		else if (!httpParser.GetUrl().compare("/test")) {
			std::string sBody;
			httpResponse.Init(200, httpParser.IsHttp11(), httpParser.IsConnectionClose());
			sBody = httpParser.IsGet() ? "GET " : "POST ";
			sBody += httpParser.GetUrl();
			sBody += httpParser.IsHttp11() ? " HTTP/1.1" : "HTTP/1.0";
			sBody += "\r\n";
			std::list<TParam> params = httpParser.GetParams();
			std::list<TParam>::iterator it = params.begin();
			while (it != params.end()) {
				sBody += (*it).paramName;
				sBody += " = ";
				sBody += (*it).paramValue;
				sBody += "\r\n";
				it++;
			}
			if (!httpResponse.Send(socket, sBody.data(), sBody.size()))
				break;
		} else {
			httpResponse.Init(404, httpParser.IsHttp11(), httpParser.IsConnectionClose());
			if (!httpResponse.Send(socket, NULL, 0))
				break;
		}

		requestHandler.CheckForRestart();

		if (httpParser.IsConnectionClose()) {
			close(socket);
			break;
		}
	}

	free(data);
	close(socket);
}
