/*
 * DnsSrv.hpp
 *
 *  Created on: 15.04.2017
 *      Author: bernd
 */

#ifndef MAIN_DNSSRV_H_
#define MAIN_DNSSRV_H_

#include <mongoose.h>

class DnsSrv {
public:
	DnsSrv();
	virtual ~DnsSrv();
	void start();
	void EventHandler(struct mg_connection *nc, int ev, void *ev_data);

private:
	in_addr_t s_our_ip_addr;
};

#endif /* MAIN_DNSSRV_H_ */
