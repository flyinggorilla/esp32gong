/*
 * Esp32Gong.h
 *
 *  Created on: 05.04.2017
 *      Author: bernd
 */

#ifndef MAIN_ESP32GONG_H_
#define MAIN_ESP32GONG_H_


class Esp32Gong {
public:
	Esp32Gong();
	virtual ~Esp32Gong();

	void Start();

	void TaskWebServer();
	void TaskResetButton();
	void TaskDnsServer();

	void IndicateApiCall() 	{ mbApiCallReceived = true; };

private:
	bool mbButtonPressed;
	bool mbApiCallReceived;

private:


};




#endif /* MAIN_ESP32GONG_H_ */
