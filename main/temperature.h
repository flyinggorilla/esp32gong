/*
 * undocumented.h
 *
 * see http://wiki.jackslab.org/ESP32_Onchip_Sensor
 *
 */

#ifndef MAIN_TEMPERATURE_H_
#define MAIN_TEMPERATURE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * undocumented functions
 */
uint8_t temprature_sens_read();

#ifdef __cplusplus
}
#endif


/* The temperature sensor has a range of -40°C to 125°C.
 * The absolute sensor results vary by chip. User calibration increases precision.
 */

float esp32_temperature() {
	return (float)temprature_sens_read() * (125.0+40.0) / 255.0 - 40.0;
}

#endif /* MAIN_TEMPERATURE_H_ */
