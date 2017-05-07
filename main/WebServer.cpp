#include "WebServer.h"
#include "Esp32gong.h"
#include "config.h"
#include "HttpRequestParser.h"
#include "HttpResponse.h"
#include "DynamicRequestHandler.h"
#include <lwip/sockets.h>
#include <esp_log.h>
#include <string.h>

#include "sdkconfig.h"
#include "fontwoff.h"
#include "fontttf.h"
#include "fontsvg.h"
#include "fonteot.h"
#include "indexhtml.h"
#include "keypem.h"
#include "certpem.h"

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

static char tag[] = "WebServer";

extern Esp32Gong esp32gong;

extern const unsigned char cacert_pem_start[] asm("_binary_cacert_pem_start");
extern const unsigned char cacert_pem_end[] asm("_binary_cacert_pem_end");
const unsigned int cacert_pem_bytes = cacert_pem_end - cacert_pem_start;

extern const unsigned char prvtkey_pem_start[] asm("_binary_prvtkey_pem_start");
extern const unsigned char prvtkey_pem_end[] asm("_binary_prvtkey_pem_end");
const unsigned int prvtkey_pem_bytes = prvtkey_pem_end - prvtkey_pem_start;

struct TServerSocketPair {
	WebServer* pServer;
	int socket;
};

void request_handler_function(void *pvParameter);

//------------------------------------------------------------------

WebServer::WebServer() {
	mpSslCtx = NULL;
}

WebServer::~WebServer() {
	SSL_CTX_free(mpSslCtx);
}

bool WebServer::Start() {
	__uint16_t port;
	struct sockaddr_in clientAddress;
	struct sockaddr_in serverAddress;

	if (esp32gong.GetConfig().muWebServerPort)
		port = esp32gong.GetConfig().muWebServerPort;
	else
		port = esp32gong.GetConfig().mbWebServerUseSsl ? 443 : 80;

	if (esp32gong.GetConfig().mbWebServerUseSsl) {

		if (!mpSslCtx) {
			mpSslCtx = SSL_CTX_new(TLS_server_method());
			if (!mpSslCtx) {
				ESP_LOGE(tag, "SSL_CTX_new: %s", strerror(errno));
				return false;
			}
			if (!SSL_CTX_use_certificate_ASN1(mpSslCtx, cacert_pem_bytes, cacert_pem_start)) {
				//if (!SSL_CTX_use_certificate_ASN1(mpSslCtx, sizeof(certpem_h), (const unsigned char*)certpem_h)){
				ESP_LOGE(tag, "SSL_CTX_use_certificate_ASN1: %s", strerror(errno));
				SSL_CTX_free(mpSslCtx);
				mpSslCtx = NULL;
				return false;
			}
			if (!SSL_CTX_use_PrivateKey_ASN1(0, mpSslCtx, prvtkey_pem_start, prvtkey_pem_bytes)) {
				//if (!SSL_CTX_use_PrivateKey_ASN1(0, mpSslCtx, (const unsigned char*)keypem_h, sizeof(keypem_h))){
				ESP_LOGE(tag, "SSL_CTX_use_PrivateKey_ASN1: %s", strerror(errno));
				SSL_CTX_free(mpSslCtx);
				mpSslCtx = NULL;
				return false;
			}
			port = 443;
		}
	}

	// Create a socket that we will listen upon.
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		ESP_LOGE(tag, "socket: %d %s", sock, strerror(errno));
		return false;
	}

	// Bind our server socket to a port.
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(port);
	int rc = bind(sock, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
	if (rc < 0) {
		ESP_LOGE(tag, "bind: %d %s", rc, strerror(errno));
		close(sock);
		return false;
	}

	// Flag the socket as listening for new connections.
	rc = listen(sock, 5);
	if (rc < 0) {
		ESP_LOGE(tag, "listen: %d %s", rc, strerror(errno));
		close(sock);
		return false;
	}
	printf("started listening\n");

	while (1) {

		// Listen for a new client connection.
		socklen_t clientAddressLength = sizeof(clientAddress);
		int clientSock = accept(sock, (struct sockaddr *) &clientAddress, &clientAddressLength);
		if (clientSock < 0) {
			ESP_LOGE(tag, "accept: %d %s", clientSock, strerror(errno));
			close(sock);
			return false;
		}
		ESP_LOGD(tag, "new connection\n");

		TServerSocketPair* pServerSocketPair = (TServerSocketPair*) malloc(sizeof(TServerSocketPair));
		pServerSocketPair->pServer = this;
		pServerSocketPair->socket = clientSock;
		xTaskCreate(&request_handler_function, "WebSocketHandler", 10240, pServerSocketPair, 5, NULL);
	}
}

void request_handler_function(void *pvParameter) {
	TServerSocketPair* serverSocket = (TServerSocketPair*) pvParameter;
	serverSocket->pServer->WebRequestHandler(serverSocket->socket);
	delete serverSocket;
	vTaskDelete(NULL);
}

void WebServer::WebRequestHandler(int socket) {

	// We now have a new client ...
	int total = 1024;
	char *data = (char*) malloc(total);
	HttpRequestParser httpParser(socket);
	HttpResponse httpResponse;
	DynamicRequestHandler requestHandler;
	SSL* ssl = NULL;

	if (mpSslCtx) {
		ssl = SSL_new(mpSslCtx);
		if (!ssl) {
			ESP_LOGE(tag, "SSL_new: %s", strerror(errno));
			goto EXIT;
		}

		SSL_set_fd(ssl, socket);

		if (!SSL_accept(ssl)) {
			ESP_LOGE(tag, "SSL_accept: %s", strerror(errno));
			goto EXIT;
		}
	}
	ESP_LOGD(tag, "Socket Accepted");

	while (1) {
		httpParser.Init();

		while (1) {
			ssize_t sizeRead;
			if (ssl)
				sizeRead = SSL_read(ssl, data, total);
			else
				sizeRead = recv(socket, data, total, 0);

			if (sizeRead <= 0) {
				ESP_LOGE(tag, "Connection closed during parsing");
				goto EXIT;
			}
			if (!httpParser.ParseRequest(data, sizeRead)) {
				ESP_LOGE(tag, "HTTP Parsing error: %d", httpParser.GetError());
				goto EXIT;
			}
			if (httpParser.RequestFinished()) {
				break;
			}
		}

		ESP_LOGD(tag, "Request parsed: %s", httpParser.GetUrl().data());

		if (ssl)
			httpResponse.Init(ssl, httpParser.IsHttp11(), httpParser.IsConnectionClose());
		else
			httpResponse.Init(socket, httpParser.IsHttp11(), httpParser.IsConnectionClose());

		if (!httpParser.GetUrl().compare("/") || !httpParser.GetUrl().compare("/index.html")) {
			httpResponse.AddHeader("Content-Type: text/html");
			httpResponse.AddHeader("Content-Encoding: gzip");
			if (!httpResponse.Send(indexhtml_h, sizeof(indexhtml_h)))
				break;
		} else if (!httpParser.GetUrl().compare("/fonts/material-design-icons.woff")) {
			if (!httpResponse.Send(fontwoff_h, sizeof(fontwoff_h)))
				break;
		} else if (!httpParser.GetUrl().compare("/fonts/material-design-icons.ttf")) {
			if (!httpResponse.Send(fontttf_h, sizeof(fontttf_h)))
				break;
		} else if (!httpParser.GetUrl().compare("/fonts/material-design-icons.eot")) {
			if (!httpResponse.Send(fonteot_h, sizeof(fonteot_h)))
				break;
		} else if (!httpParser.GetUrl().compare("/fonts/material-design-icons.svg")) {
			if (!httpResponse.Send(fontsvg_h, sizeof(fontsvg_h)))
				break;
		} /*else if (!httpParser.GetUrl().compare("/api")) {
			std::string sBody;
			if (!requestHandler.HandleApiRequest(httpParser.GetParams(), httpResponse))
				break;
		} else if (!httpParser.GetUrl().compare("/apilist")) {
			std::string sBody;
			if (!requestHandler.HandleApiListRequest(httpParser.GetParams(), httpResponse))
				break;
		} else if (!httpParser.GetUrl().compare("/apiedit")) {
			std::string sBody;
			if (!requestHandler.HandleApiEditRequest(httpParser.GetParams(), httpResponse))
				break;
		} */ else if (!httpParser.GetUrl().compare("/info")) {
			if (!requestHandler.HandleInfoRequest(httpParser.GetParams(), httpResponse))
				break;
		} else if (!httpParser.GetUrl().compare("/config")) {
			if (!requestHandler.HandleConfigRequest(httpParser.GetParams(), httpResponse))
				break;
		} else if (!httpParser.GetUrl().compare("/firmware")) {
			if (!requestHandler.HandleFirmwareRequest(httpParser.GetParams(), httpResponse))
				break;
		}

		else if (!httpParser.GetUrl().compare("/test")) {
			std::string sBody;
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
			if (!httpResponse.Send(sBody.data(), sBody.size()))
				break;
		} else {
			httpResponse.SetRetCode(404);
			if (!httpResponse.Send(NULL, 0))
				break;
		}

		requestHandler.CheckForRestart();

		if (httpParser.IsConnectionClose()) {
			close(socket);
			break;
		}
	}

	EXIT: if (ssl)
		SSL_free(ssl);

	free(data);
	close(socket);
}
