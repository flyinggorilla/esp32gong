#include "CriticalSection.h"

//------------------------------------------------------------------

CriticalSection::CriticalSection() {
	myMutex = portMUX_INITIALIZER_UNLOCKED;
	mbFree = true;
}

CriticalSection::~CriticalSection() {
}

bool CriticalSection::Enter(__uint16_t ticks){
	while (true){
		taskENTER_CRITICAL(&myMutex);
		if (mbFree){
			mbFree = false;
			taskEXIT_CRITICAL(&myMutex);
			return true;
		}
		taskEXIT_CRITICAL(&myMutex);
		if (ticks && !--ticks)
			return false;
		vTaskDelay(1);
	}
}

void CriticalSection::Leave(){
	taskENTER_CRITICAL(&myMutex);
	mbFree = true;
	taskEXIT_CRITICAL(&myMutex);
}

