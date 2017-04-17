/*
 * WebClient.cpp
 *
 *  Created on: 09.04.2017
 *      Author: bernd
 */

#include "WebClient.hpp"

#include "esp_log.h"
#include "sdkconfig.h"

#define LOGTAG "WebClient"

WebClient::WebClient() {

	mg_mgr_init(&mgr, this);

}

WebClient::~WebClient() {
	mg_mgr_free(&mgr);
}

void WebClient::ConnectEvent(int errorcode) {
	if (errorcode != 0) {
		ESP_LOGE(LOGTAG, "connect() failed: %s",
				strerror(errorcode));
		exit_flag = 1;
	} else {
		if (!downloadHandler) {
			return;
		}
		downloadHandler->OnReceiveBegin();
	}
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

	//int received = * ((int*)(ev_data));


	downloadHandler->OnReceiveData(data, len); //OR received???
	/*
	 MG_EV_RECV: New data is received and appended to the end of recv_mbuf. void *ev_data is int *num_received_bytes. Typically, event handler should check received data in nc->recv_mbuf, discard processed data by calling mbuf_remove(), set connection flags nc->flags if necessary (see struct mg_connection) and write data the remote peer by output functions like mg_send().
	 WARNING: Mongoose uses realloc() to expand the receive buffer. It is the user's responsibility to discard processed data from the beginning of the receive buffer, note the mbuf_remove() call in the example above.

	 struct mg_connection::recv_mbuf respectively. When data arrives, Mongoose appends received data to the recv_mbuf and triggers an MG_EV_RECV event. The user may send data back by calling one of the output functions, like mg_send() or mg_printf(). Output functions append data to the send_mbuf. When Mongoose successfully writes data to the socket, it discards data from struct mg_connection::send_mbuf and sends an MG_EV_SEND event. When the connection is closed, an MG_EV_CLOSE event is sent.

	 */



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

static void webclientEventHandler(struct mg_connection *nc, int event_id, void *ev_data) {
	struct http_message *hm = (struct http_message *) ev_data;

	WebClient* wc = (WebClient*) nc->user_data;

	ESP_LOGI(LOGTAG, "event handler: %i", event_id);

	switch (event_id) {
	case MG_EV_CONNECT:
		ESP_LOGI(LOGTAG, "event handler: connect");
		wc->ConnectEvent(*(int *) ev_data);
		break;
	case MG_EV_RECV:
		wc->ReceiveEvent(nc->recv_mbuf.buf, nc->recv_mbuf.len);
		mbuf_remove(&nc->recv_mbuf, nc->recv_mbuf.len);
		ESP_LOGI(LOGTAG, "event handler: receive");
	case MG_EV_HTTP_REPLY:
		ESP_LOGI(LOGTAG, "event handler: reply");
		wc->ReplyEvent();
		nc->flags |= MG_F_CLOSE_IMMEDIATELY;
		//ESP_LOGI(LOGTAG, "http message: %s", hm->message.p);
		//ESP_LOGI(LOGTAG, "http body: %s", hm->body.p);
		break;
	case MG_EV_CLOSE:
		ESP_LOGI(LOGTAG, "event handler: close");
		wc->CloseEvent();
		break;
	default:
		ESP_LOGI(LOGTAG, "event handler: unknown");
		break;
	}
}

bool WebClient::request(const char* url, DownloadHandler* downloadHandler) {

	struct mg_connect_opts opts;
	memset(&opts, 0, sizeof(opts));
	opts.user_data = this;

	exit_flag = 0;
	ESP_LOGI(LOGTAG, "Requesting %s", url);
	this->downloadHandler = downloadHandler;
	struct mg_connection* nc = mg_connect_http_opt(&mgr,
												webclientEventHandler,
												opts,
												url,
												NULL, // extra headers
												NULL); // post data

	if (nc == NULL) {
		ESP_LOGE(LOGTAG, "Error connecting!");
		return false;
	}
	ESP_LOGI(LOGTAG, "Polling eventloop!");

	while (exit_flag == 0) {
		mg_mgr_poll(&mgr, 1000);
	}

	return true; //TODO

}
