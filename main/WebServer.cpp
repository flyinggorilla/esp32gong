/*
 * WebServer.cpp
 *
 *  Created on: 10.04.2017
 *      Author: bernd
 */

#include "sdkconfig.h"

#include "WebServer.hpp"
#include "Esp32Gong.h"

#include "esp_event.h"
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <string>
#include "time.h"
#include "indexhtml.h"
#include "fontwoff.h"
#include "fontttf.h"
#include "fontsvg.h"
#include "fonteot.h"
#include "cJSON.h"
#include "UrlParser.hpp"

#include "I2SPlayer.hpp"
#include "Ota.hpp"
#include "SpiffsFileSystem.hpp"
#include "Wifi.hpp"
#include "Config.hpp"

#define EXAMPLE_SERVER_IP "10.10.29.100"
#define EXAMPLE_SERVER_PORT "80"
#define EXAMPLE_FILENAME "esp32gong.bin"

extern I2SPlayer musicPlayer;
extern Wifi wifi;
extern Config config;

WebServer::WebServer() {
	// TODO Auto-generated constructor stub

}

WebServer::~WebServer() {
	//free(this->printedJson);
}

#define LOGTAG "webserver"

char* json_unformatted;

const static char http_json_hdr[] = "Content-type: text/json\r\n"
		"cache-control: private, max-age=0, no-cache, no-store";
const static char http_htmlgzip_hdr[] = "Content-type: text/html\r\n"
		"Content-Encoding: gzip";
const static char http_font_hdr[] = "Content-type: application/octet-stream";
const static char http_html_hdr[] = "Content-type: text/html";
const static char http_default_hdr[] = "Content-type: text/html\r\n"
		"cache-control: private, max-age=0, no-cache, no-store";

/*
 * @return json string - make sure to free() it afterwards!
 */
char* WebServer::createInfoJson() {
	/*if (this->printedJson) {
	 free(this->printedJson);
	 this->printedJson = NULL;
	 }*/
	cJSON *json;
	json = cJSON_CreateObject();
	char sbuf[128];

	cJSON_AddStringToObject(json, "esp-idf", esp_get_idf_version());
	cJSON_AddNumberToObject(json, "heap", esp_get_free_heap_size());
	cJSON_AddStringToObject(json, "ssid", config.msSTASsid.c_str());

	tcpip_adapter_ip_info_t ip_info;
	ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));
	sprintf(sbuf, IPSTR, IP2STR(&ip_info.ip));
	cJSON_AddStringToObject(json, "ipaddress", sbuf);
	sprintf(sbuf, IPSTR, IP2STR(&ip_info.gw));
	cJSON_AddStringToObject(json, "ipgateway", sbuf);
	sprintf(sbuf, IPSTR, IP2STR(&ip_info.netmask));
	cJSON_AddStringToObject(json, "ipsubnetmask", sbuf);
	sprintf(sbuf, IPSTR, IP2STR(&ip_info.gw));
	cJSON_AddStringToObject(json, "ipdns", sbuf);
	cJSON_AddStringToObject(json, "hostname", config.msHostname.c_str());
	uint8_t mac[6];
	ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA, mac));
	sprintf(sbuf, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	cJSON_AddStringToObject(json, "macaddress", sbuf);
	ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &ip_info));
	sprintf(sbuf, IPSTR, IP2STR(&ip_info.ip));
	cJSON_AddStringToObject(json, "apipaddress", sbuf);

	//TODO list only when in AP mode!!!
	wifi_sta_list_t apstations;
	if (ESP_OK == esp_wifi_ap_get_sta_list(&apstations)) {
		sprintf(sbuf, "%i", apstations.num);
	} else {
		sprintf(sbuf, "n/a");
	}
	cJSON_AddStringToObject(json, "apconnectedstations", sbuf);
	bool isautoconnect;
	ESP_ERROR_CHECK(esp_wifi_get_auto_connect(&isautoconnect));
	cJSON_AddStringToObject(json, "wifiautoconnect", isautoconnect ? "on" : "off");
	cJSON_AddStringToObject(json, "firmwareversion", FIRMWARE_VERSION);

	return cJSON_PrintUnformatted(json);
}

const char* WebServer::mgEventToString(int event) {
	switch (event) {
	case MG_EV_CONNECT:
		return "MG_EV_CONNECT";
	case MG_EV_ACCEPT:
		return "MG_EV_ACCEPT";
	case MG_EV_CLOSE:
		return "MG_EV_CLOSE";
	case MG_EV_SEND:
		return "MG_EV_SEND";
	case MG_EV_RECV:
		return "MG_EV_RECV";
	case MG_EV_HTTP_REQUEST:
		return "MG_EV_HTTP_REQUEST";
	case MG_EV_HTTP_REPLY:
		return "MG_EV_HTTP_REPLY";
	case MG_EV_WEBSOCKET_HANDSHAKE_REQUEST:
		return "MG_EV_WEBSOCKET_HANDSHAKE_REQUEST";
	case MG_EV_WEBSOCKET_HANDSHAKE_DONE:
		return "MG_EV_WEBSOCKET_HANDSHAKE_DONE";
	case MG_EV_WEBSOCKET_FRAME:
		return "MG_EV_WEBSOCKET_FRAME";
	}
	return "UNKNOWN";
} //eventToString

// Convert a Mongoose string type to a string.
char* WebServer::mgStrToStr(struct mg_str mgStr) {
	char *retStr = (char *) malloc(mgStr.len + 1);
	memcpy(retStr, mgStr.p, mgStr.len);
	retStr[mgStr.len] = 0;
	return retStr;
} // mgStrToStr

struct mg_str fileUploadHandler(struct mg_connection *c, struct mg_str file_name) {
	// Return the same filename. Do not actually do this except in test!
	// fname is user-controlled and needs to be sanitized.
	return file_name;
}

/* When compiled with MG_ENABLE_HTTP_STREAMING_MULTIPART, Mongoose parses
 * multipart requests and splits them into separate events:
 * - MG_EV_HTTP_MULTIPART_REQUEST: Start of the request.
 *   This event is sent before body is parsed. After this, the user
 *   should expect a sequence of PART_BEGIN/DATA/END requests.
 *   This is also the last time when headers and other request fields are
 *   accessible.
 * - MG_EV_HTTP_PART_BEGIN: Start of a part of a multipart message.
 *   Argument: mg_http_multipart_part with var_name and file_name set
 *   (if present). No data is passed in this message.
 * - MG_EV_HTTP_PART_DATA: new portion of data from the multipart message.
 *   Argument: mg_http_multipart_part. var_name and file_name are preserved,
 *   data is available in mg_http_multipart_part.data.
 * - MG_EV_HTTP_PART_END: End of the current part. var_name, file_name are
 *   the same, no data in the message. If status is 0, then the part is
 *   properly terminated with a boundary, status < 0 means that connection
 *   was terminated.
 * - MG_EV_HTTP_MULTIPART_REQUEST_END: End of the multipart request.
 *   Argument: mg_http_multipart_part, var_name and file_name are NULL,
 *   status = 0 means request was properly closed, < 0 means connection
 *   was terminated (note: in this case both PART_END and REQUEST_END are
 *   delivered).
 *    HTTP multipart part
 struct mg_http_multipart_part {
 const char *file_name;
 const char *var_name;
 struct mg_str data;
 int status; <0 on error
 void *user_data;
 };*/

/*
 struct file_writer_data {
 FILE *fp;
 size_t bytes_written;
 };*/

void HttpUploadHandler(struct mg_connection *nc, int event, void *p) {
	//WebServer* ws = (WebServer*) nc->user_data;
	//TODO webserver structur maybe not neede; instead create a Spiffs object for handling upload;
	struct mg_http_multipart_part *mp = (struct mg_http_multipart_part *) p;
	ESP_LOGI(LOGTAG, "Mongoose handle upload event....");

	SpiffsFile* spiffsFile = (SpiffsFile*) nc->user_data;

	switch (event) {
	case MG_EV_HTTP_PART_BEGIN: {

		ESP_LOGI(LOGTAG, "Mongoose handle upload part begin");

		if (spiffsFile == NULL) {
			spiffsFile = new SpiffsFile();
			nc->user_data = spiffsFile;
		}

		if (!spiffsFile->Open(mp->file_name)) {
			mg_printf(nc, "%s", "HTTP/1.1 500 Failed to open a file\r\n"
					"Content-Length: 0\r\n\r\n");
			nc->flags |= MG_F_SEND_AND_CLOSE;
			return;
		}

		break;
	}
	case MG_EV_HTTP_PART_DATA: {
		ESP_LOGI(LOGTAG, "Mongoose handle upload part data");

		if (spiffsFile->Write((char*) mp->data.p, mp->data.len) != mp->data.len) {
			mg_printf(nc, "%s", "HTTP/1.1 500 Failed to write to a file\r\n"
					"Content-Length: 0\r\n\r\n");
			nc->flags |= MG_F_SEND_AND_CLOSE;
			return;
		}
		//data->bytes_written += mp->data.len;
		break;
	}
	case MG_EV_HTTP_PART_END: {
		ESP_LOGI(LOGTAG, "Mongoose handle upload part end");
		mg_printf(nc, "HTTP/1.1 200 OK\r\n"
				"Content-Type: text/plain\r\n"
				"Connection: close\r\n\r\n"
				"Written %i of POST data to a temp file\n\n", spiffsFile->getBytesWritten());
		nc->flags |= MG_F_SEND_AND_CLOSE;
		spiffsFile->Close();

		delete spiffsFile;
		spiffsFile = NULL;
		nc->user_data = NULL;
		break;
	}
	default:
		ESP_LOGI(LOGTAG, "Mongoose upload handler - no default defined... add?");
	}
}

void restartTask(void* user_data) {
	ESP_LOGI(LOGTAG, "Restarting in 2secs....");
	vTaskDelay(*((int*)user_data)*1000 / portTICK_PERIOD_MS);
	esp_restart();
}

void WebServer::Restart(int seconds) {
	xTaskCreate(&restartTask, "restartTask", 2048, &seconds, 5, NULL);
}


// Mongoose event handler.
void HttpRequestHandler(struct mg_connection *nc, int ev, void *evData) {

	WebServer* ws = (WebServer*)nc->user_data;

	switch (ev) {
	case MG_EV_HTTP_PART_BEGIN:
	case MG_EV_HTTP_PART_DATA:
	case MG_EV_HTTP_PART_END:
		ESP_LOGI(LOGTAG, "HTTP PART* CALLED");
		mg_file_upload_handler(nc, ev, evData, fileUploadHandler);
		break;
	case MG_EV_HTTP_REQUEST: {

		struct http_message *message = (struct http_message *) evData;

		char *uri = WebServer::mgStrToStr(message->uri);
		char* query = WebServer::mgStrToStr(message->query_string);

		UrlParser up;
		up.parseQuery(query);

		ESP_LOGI(LOGTAG, "HTTP Request: %s", uri);

		if (strcmp(uri, "/info") == 0) {
			char* jsontext = WebServer::createInfoJson();
			ESP_LOGI(LOGTAG, "createInfoJson: '%s'", jsontext);
			int jsonlen = strlen(jsontext);
			mg_send_head(nc, 200, jsonlen, http_json_hdr);
			mg_send(nc, jsontext, jsonlen);
			free(jsontext);
		} else if (strcmp(uri, "/fonts/material-design-icons.ttf") == 0) {
			mg_send_head(nc, 200, sizeof(fontttf_h), http_font_hdr);
			mg_send(nc, fontttf_h, sizeof(fontttf_h));
		} else if (strcmp(uri, "/fonts/material-design-icons.woff") == 0) {   // woff
			mg_send_head(nc, 200, sizeof(fontwoff_h), http_font_hdr);
			mg_send(nc, fontwoff_h, sizeof(fontwoff_h));
		} else if (strcmp(uri, "/fonts/material-design-icons.svg") == 0) {
			mg_send_head(nc, 200, sizeof(fontsvg_h), http_font_hdr);
			mg_send(nc, fontsvg_h, sizeof(fontsvg_h));
		} else if (strcmp(uri, "/fonts/material-design-icons.eot") == 0) {   // woff
			mg_send_head(nc, 200, sizeof(fonteot_h), http_font_hdr);
			mg_send(nc, fonteot_h, sizeof(fonteot_h));
		} else if (strcmp(uri, "/") == 0) {
			mg_send_head(nc, 200, sizeof(indexhtml_h), http_htmlgzip_hdr);
			mg_send(nc, indexhtml_h, sizeof(indexhtml_h));
			ESP_LOGI(LOGTAG, "homepage");
		} else if (strcmp(uri, "/config") == 0) {
			ESP_LOGI(LOGTAG, "entering api evaluation");

			if (up.isKey("ssid") && up.isKey("pwd")) {
				std::string ssid(up.getValueOf("ssid"));
				std::string pwd(up.getValueOf("pwd"));
				std::string ca(up.isKey("ca") ? up.getValueOf("ca") : "");
				std::string user(up.isKey("user") ? up.getValueOf("user") : "");
				std::string hostname(up.isKey("hostname") ? up.getValueOf("hostname") : "");//
				ESP_LOGI(LOGTAG, "Setting Wifi credentials user='%s' pass='%s' user='%s' ca='%s'", ssid.c_str(),
						pwd.c_str(), user.c_str(), ca.c_str());
				if (!ssid.empty() && !pwd.empty()) {
					config.msSTASsid = ssid;
					config.msSTAPass = pwd;
					if (!hostname.empty()) {
						config.msHostname = hostname;
					}
					if (!user.empty()) {
						config.msSTAENTUser = user;
					} else {
						config.msSTAENTUser.clear();
					}
					if (!ca.empty()) {
						//ESP_LOGD("DynamicRequestHandler", "<%s>", sWifiCA);
						config.msSTAENTCA = ca;
					} else {
						config.msSTAENTCA.clear();
					}
					//ESP_LOGI(LOGTAG, "before writing config - apmode %i", (int )config.mbAPMode);
					config.mbAPMode = false;
					config.Write();
					//ESP_LOGI(LOGTAG, "after writing config - apmode %i", (int )config.mbAPMode);
					ws->Restart(2);
					char header[256];
					snprintf(header, sizeof(header)-1, "Content-type: text/html\r\nRefresh:10; url=http://%s", config.msHostname.c_str());
					char response[512];
					snprintf(response, sizeof(response)-1,
							"<html><body>New Wifi credentials successfully set! rebooting now....... browser refreshs in 10 seconds to http://%s</html></body>",
							config.msHostname.c_str());
					mg_send_head(nc, 200, strlen(response), header);
					mg_send(nc, response, strlen(response));
					ESP_LOGI(LOGTAG, "%s", header);

				}

			} else if (up.isKey("gong")) {
				musicPlayer.playAsync(); // attenuation 0=maxvolume .. 16=maxattenuation
				const char* response = "<html><body>api call - lets play music</html></body>";
				mg_send_head(nc, 200, strlen(response), http_default_hdr);
				mg_send(nc, response, strlen(response));
			} else {
				const char* response = "<html><body>INVALID API CALL</html></body>";
				mg_send_head(nc, 400, strlen(response), http_default_hdr);
				mg_send(nc, response, strlen(response));
			}
		} else if (strcmp(uri, "/firmwareupdate") == 0) {
			Ota ota;
			ota.update("http://" EXAMPLE_SERVER_IP ":" EXAMPLE_SERVER_PORT "/esp32gong/esp32gong.bin");
			// successful ota.update will reboot!
			const char* response = "<html><body>error - could not update firmware -- wrong URL? </html></body>";
			mg_send_head(nc, 400, strlen(response), http_default_hdr);
			mg_send(nc, response, strlen(response));
		} else if (strcmp(uri, "/connecttest.txt") == 0) { // microsofts query to test for connectino.... sadly the browser doesnt open automatically so far
			const char* response = "<html> <head> <title>Network Authentication Required</title> "
					"<meta http-equiv=\"refresh\" content=\"5; url=http://192.168.4.1/captive\"> "
					"</head> <body> <p>You need to <a href=\"http://192.168.4.1/\">"
					"authenticate with the local network</a> in order to gain access.</p> </body> </html>";
			const static char http_msft_hdr[] = "Content-type: text/plain\r\nLocation: http://192.168.4.1/";
			mg_send_head(nc, 511, strlen(response), http_msft_hdr);
			mg_send(nc, response, strlen(response));
		} else {
			ESP_LOGI(LOGTAG, "404 not found - %s", uri);
			const char* response = "<html><body>file not found</html></body>";
			mg_send_head(nc, 404, strlen(response), http_default_hdr);
			mg_send(nc, response, strlen(response));
		}

		nc->flags |= MG_F_SEND_AND_CLOSE;
		free(uri);
		free(query);
		break;
	}
	} // End of switch



} // End of mongoose_event_handler

// FreeRTOS task to start Mongoose.
void webserverTask(void* user_data) {
	WebServer* ws = (WebServer*) user_data;
	ESP_LOGI(LOGTAG, "Mongoose task starting");
	struct mg_mgr mgr;
	ESP_LOGI(LOGTAG, "Mongoose: Starting setup");
	mg_mgr_init(&mgr, ws);
	ESP_LOGI(LOGTAG, "Mongoose: Successfully inited");
	struct mg_bind_opts opts;
	 opts.error_string = NULL;
	 opts.flags = 0;
	 opts.iface = NULL;
	 opts.user_data = user_data;

	while (true) {
		struct mg_connection *nc = NULL;
		ESP_LOGI(LOGTAG, "Binding Webserver to port 80");
		while (!nc) {
			nc = mg_bind_opt(&mgr, ":80", HttpRequestHandler, opts);
			if (!nc) {
				ESP_LOGW(LOGTAG, "Binding Webserver failed, retrying in 5 seconds....");
				vTaskDelay(5000 / portTICK_PERIOD_MS);
			}
		}
		ESP_LOGI(LOGTAG, "Webserver successfully bound");
		mg_register_http_endpoint(nc, "/update", HttpUploadHandler);
		mg_set_protocol_http_websocket(nc);

		while (true) { // this loop does the actual work and calls the eventhandler
			mg_mgr_poll(&mgr, 1000);
		}
		mg_mgr_free(&mgr);
	}
} // mongooseTask

void WebServer::start() {
	//gpio_pad_select_gpio(LED_BUILTIN);
	ESP_LOGI(LOGTAG, "Webserver starting...");

	// Set the GPIO as a push/pull output
	//gpio_set_direction(LED_BUILTIN, GPIO_MODE_OUTPUT);
	//xTaskCreate(&generate_json, "json", 2048, NULL, 5, NULL);
	//xTaskCreatePinnedToCore(&webserverTask, "webserverTask", 20000, NULL, 5, NULL,0);
	xTaskCreate(&webserverTask, "webserverTask", 4096, this, 5, NULL);
	//xTaskCreate(&http_server, "http_server", 2048, NULL, 5, NULL);
	//return 0;
}
