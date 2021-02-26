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
		portENTER_CRITICAL(&myMutex);
		if (mbFree){
			mbFree = false;
			portEXIT_CRITICAL(&myMutex);
			return true;
		}
		portEXIT_CRITICAL(&myMutex);
		if (ticks && !--ticks)
			return false;
		vTaskDelay(1);
	}
}

void CriticalSection::Leave(){
	portENTER_CRITICAL(&myMutex);
	mbFree = true;
	portEXIT_CRITICAL(&myMutex);
}

