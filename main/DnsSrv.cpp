/*
 * DnsSrv.cpp
 *
 *  Created on: 15.04.2017
 *      Author: bernd
 */

#include "DnsSrv.hpp"
#include <esp_log.h>
#include <stdio.h>

static const char LOGTAG[] = "DnsSrv";

DnsSrv::DnsSrv() {
	// TODO Auto-generated constructor stub

}

DnsSrv::~DnsSrv() {
	// TODO Auto-generated destructor stub
}

static void dnsEventHandler(struct mg_connection *nc, int ev, void *ev_data) {
	DnsSrv* dnsSrv = (DnsSrv*) nc->user_data;
	dnsSrv->EventHandler(nc, ev, ev_data);
}

void DnsSrv::EventHandler(struct mg_connection *nc, int ev, void *ev_data) {
	struct mg_dns_message *msg;
	struct mg_dns_resource_record *rr;
	struct mg_dns_reply reply;
	int i;

	switch (ev) {
	case MG_DNS_MESSAGE: {
		struct mbuf reply_buf;
		mbuf_init(&reply_buf, 512);
		msg = (struct mg_dns_message *) ev_data;
		reply = mg_dns_create_reply(&reply_buf, msg);

		for (i = 0; i < msg->num_questions; i++) {
			char rname[512];
			rr = &msg->questions[i];
			mg_dns_uncompress_name(msg, &rr->name, rname, sizeof(rname) - 1);
			ESP_LOGI(LOGTAG, "Q type %d name %s", rr->rtype, rname);
			in_addr_t iaddr;
			if (strcmp(rname, " www.msftconnecttest.com") == 0) {
				//iaddr = inet_addr("131.107.255.255");
				ESP_LOGI(LOGTAG, "detected microsoft DNS request - returning 131.107.255.255")
			} else if (strcmp(rname, "dns.msftncsi.com") == 0) {
				//iaddr = inet_addr("131.107.255.255");
				ESP_LOGI(LOGTAG, "detected microsoft DNS request - returning 131.107.255.255")
			} else {
				iaddr = inet_addr("192.168.4.1");
				if (rr->rtype == MG_DNS_A_RECORD) {
					mg_dns_reply_record(&reply, rr, NULL, rr->rtype, 10, &iaddr, sizeof(in_addr_t));
				}
			}

		}

		/*
		 * We don't set the error flag even if there were no answers
		 * matching the MG_DNS_A_RECORD query type.
		 * This indicates that we have (synthetic) answers for MG_DNS_A_RECORD.
		 * See http://goo.gl/QWvufr for a distinction between NXDOMAIN and NODATA.
		 */

		mg_dns_send_reply(nc, &reply);
		nc->flags |= MG_F_SEND_AND_CLOSE;
		mbuf_free(&reply_buf);
		break;
	}
	}
}

void DnsSrv::start() {
	struct mg_mgr mgr;
	struct mg_connection *nc;

	s_our_ip_addr = inet_addr("127.0.0.1");

	mg_mgr_init(&mgr, this);
	ESP_LOGI(LOGTAG, "binding DNS server");
	nc = mg_bind(&mgr, "udp://:53", dnsEventHandler);
	if (nc == NULL) {
		ESP_LOGE(LOGTAG, "could not bind DNS server to udp://:53 ");
		mg_mgr_free(&mgr);
		return;
	}
	ESP_LOGI(LOGTAG, "bound DNS server to udp://:53 ");
	mg_set_protocol_dns(nc);
	ESP_LOGI(LOGTAG, "protocol set ");

	while (true) {
		mg_mgr_poll(&mgr, 1000);
	}
	mg_mgr_free(&mgr);

}
