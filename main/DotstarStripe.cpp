#include <freertos/FreeRTOS.h>
#include "DotstarStripe.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include <esp_log.h>

DotstarStripe::DotstarStripe(__uint8_t count, gpio_num_t cl, gpio_num_t dt) {
	clock = cl;
	data = dt;
	ledCount = count;
	startPos = 0;
	colorRed = (__uint8_t*)malloc(count);
	colorGreen =  (__uint8_t*)malloc(count);
	colorBlue =  (__uint8_t*)malloc(count);

	InitColor(0, 0, 0);
}

DotstarStripe::~DotstarStripe() {
	free(colorRed);
	free(colorGreen);
	free(colorBlue);
}


void DotstarStripe::InitColor(__uint8_t r, __uint8_t g, __uint8_t b){
	for(__uint8_t i=0 ; i<ledCount ; i++){
		colorRed[i] = r;
		colorGreen[i] = g;
		colorBlue[i] = b;
	}
}

void DotstarStripe::SetLeds(__uint8_t pos, __uint8_t count, __uint8_t r, __uint8_t g, __uint8_t b){

	if (count > ledCount)
		count = ledCount;
	for(__uint8_t i=0 ; i<count ; i++){
		__uint8_t p = (pos + i) % ledCount;
		colorRed[p] = r;
		colorGreen[p] = g;
		colorBlue[p] = b;
	}

}

void DotstarStripe::Show(){
 __uint8_t i;

  for (i=0 ; i<4 ; i++)
	SendByte(0);

  for (i=0 ; i<ledCount ; i++)
  {
	 SendByte(0xff);
	 SendByte(getBlue(i));
	 SendByte(getGreen(i));
	 SendByte(getRed(i));
  }
  for (i=0 ; i<4 ; i++)
	SendByte(0xff);
}

//---------------------------------------------------------------------------

void DotstarStripe::SendByte(__uint8_t out){
  __uint8_t n = 8;

  while (n--)
  {
    if (out & 0x80)
      gpio_set_level(data, 1);
    else
      gpio_set_level(data, 0);
	  __asm__ __volatile__("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;"); //1 nop is about 4ns
	  __asm__ __volatile__("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;");
	  __asm__ __volatile__("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;");
	  __asm__ __volatile__("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;");
	  __asm__ __volatile__("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;");
	  __asm__ __volatile__("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;");
	  __asm__ __volatile__("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;");
	
    gpio_set_level(clock, 1);
    out <<= 1;
	  __asm__ __volatile__("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;");
	  __asm__ __volatile__("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;");
	  __asm__ __volatile__("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;");
    __asm__ __volatile__("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;");
    __asm__ __volatile__("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;");
    gpio_set_level(clock, 0);
	  __asm__ __volatile__("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;");
	  __asm__ __volatile__("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;");
	
  }
}


