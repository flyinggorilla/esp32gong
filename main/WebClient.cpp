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

	mg_mgr_init(&mgr, NULL);

}

WebClient::~WebClient() {
	mg_mgr_free(&mgr);
}

static void ev_handler(struct mg_connection *nc, int event_id, void *ev_data) {
	struct http_message *hm = (struct http_message *) ev_data;

	//WebClient* wc = (WebClient*) nc->user_data;

	DownloadHandler* dh = (DownloadHandler*) nc->user_data;

	switch (event_id) {
	case MG_EV_CONNECT:
		if (*(int *) ev_data != 0) {
			ESP_LOGE(LOGTAG, "connect() failed: %s",
					strerror(*(int * ) ev_data));
			//wc->exit_flag = 1;
		} else {
			if (!dh) {
				return;
			}
			dh->OnReceiveBegin();
		}
		break;
	case MG_EV_RECV:
		if (!dh) {
			return;
		}

		//int received = * ((int*)(ev_data));

		dh->OnReceiveData(nc->recv_mbuf.buf, nc->recv_mbuf.len); //OR received???
		/*
		 MG_EV_RECV: New data is received and appended to the end of recv_mbuf. void *ev_data is int *num_received_bytes. Typically, event handler should check received data in nc->recv_mbuf, discard processed data by calling mbuf_remove(), set connection flags nc->flags if necessary (see struct mg_connection) and write data the remote peer by output functions like mg_send().
		 WARNING: Mongoose uses realloc() to expand the receive buffer. It is the user's responsibility to discard processed data from the beginning of the receive buffer, note the mbuf_remove() call in the example above.

		 struct mg_connection::recv_mbuf respectively. When data arrives, Mongoose appends received data to the recv_mbuf and triggers an MG_EV_RECV event. The user may send data back by calling one of the output functions, like mg_send() or mg_printf(). Output functions append data to the send_mbuf. When Mongoose successfully writes data to the socket, it discards data from struct mg_connection::send_mbuf and sends an MG_EV_SEND event. When the connection is closed, an MG_EV_CLOSE event is sent.

		 */

		mbuf_remove(&nc->recv_mbuf, nc->recv_mbuf.len);
	case MG_EV_HTTP_REPLY:
		nc->flags |= MG_F_CLOSE_IMMEDIATELY;
		ESP_LOGI(LOGTAG, "http message: %s", hm->message.p)
		;
		ESP_LOGI(LOGTAG, "http body: %s", hm->body.p)
		;
		//wc->exit_flag = 1;
		break;
	case MG_EV_CLOSE:
		/*if (wc->exit_flag == 0) {
			ESP_LOGI(LOGTAG, "server closed connection");
			wc->exit_flag = 1;
		}*/
		if (!dh) {
			return;
		}
		dh->OnReceiveEnd();
		break;
	default:
		break;
	}
}

bool WebClient::request(const char* url, DownloadHandler* downloadHandler) {

	struct mg_connect_opts opts;
	opts.user_data = downloadHandler;
	opts.flags = 0;
	opts.iface = NULL;
	opts.error_string = NULL;
	//opts.ssl_ca_cert = NULL;
	//opts.ssl_cert = NULL;
	//opts.ssl_cipher_suites = NULL;
	//opts.ssl_key = "*"; // Verify server certificate using this CA bundle. If set to "*", then SSL is enabled but no cert verification is performed.
	//opts.ssl_psk_identity = NULL;
	//opts.ssl_psk_key = NULL;
	//opts.ssl_server_name = "*"; // disable name verification (otherwise use NULL, then connection host name is used)

	//struct mg_connection* mgc =
	mg_connect_http_opt(&mgr, ev_handler, opts, url,
	NULL, // extra headers
			NULL); // post data

	//mg_connect_http(&mgr, ev_handler, argv[i], NULL, NULL);
	while (exit_flag == 0) {
		mg_mgr_poll(&mgr, 1000);
	}

	return true; //TODO

}
