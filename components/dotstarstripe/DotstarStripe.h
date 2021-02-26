#ifndef MAIN_DOTSTARSTRIPE_H_
#define MAIN_DOTSTARSTRIPE_H_

#include "driver/gpio.h"

class DotstarStripe {
public:
	DotstarStripe(__uint8_t count, gpio_num_t cl, gpio_num_t dt);
	virtual ~DotstarStripe();

	void InitColor(__uint8_t r, uint8_t g, __uint8_t b);
	void SetLeds(__uint8_t pos, __uint8_t count, __uint8_t r, __uint8_t g, __uint8_t b);

	void Show();

	uint8_t getCount()				{ return ledCount; };

	uint8_t getRed(uint8_t pos) 	{ return colorRed[(pos + startPos) % ledCount]; };
	uint8_t getGreen(uint8_t pos)	{ return colorGreen[(pos + startPos) % ledCount]; };
	uint8_t getBlue(uint8_t pos)	{ return colorBlue[(pos + startPos) % ledCount]; };

	void SetStartPos(uint8_t pos)  	{ startPos = pos % ledCount; };

private:
	void SendByte(uint8_t out);

private:
	gpio_num_t clock;
	gpio_num_t data;
	__uint8_t ledCount;
	__uint8_t startPos;
	__uint8_t* colorRed;
	__uint8_t* colorGreen;
	__uint8_t* colorBlue;
};

#endif /* MAIN_DOTSTARSTRIPE_H_ */
