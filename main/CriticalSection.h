#ifndef MAIN_CRITICALSECTION_H_
#define MAIN_CRITICALSECTION_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class CriticalSection {
public:
	CriticalSection();
	virtual ~CriticalSection();

	bool Enter(__uint16_t ticks);
	void Leave();

private:
	portMUX_TYPE myMutex;
	bool mbFree;

};

#endif 
