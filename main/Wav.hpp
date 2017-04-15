/*
 * Wav.h
 *
 *  Created on: 11.03.2017
 *      Author: bernd
 */

#include <stdint.h>
#include <stdio.h>

#ifndef MAIN_WAV_H_
#define MAIN_WAV_H_

class Wav {
public:
	Wav();
	virtual ~Wav();

	// WAV mono 8kHz = 125microseconds per sample
	// 8bit sample devided by 29 to convert to 9 possible PWM states 0.....8 HIG

	/*
	 * @brief parses the wav data on first call; rewinds iterator to the beginning on subsequent calls
	 *
	 * @return false if wav format is invalid or not supported
	 *
	 */
	bool init(const char* wavdata, unsigned int size);

	/*
	 * @brief	retrieves sample rate of wav file
	 * @return	sample rate of wav file e.g. 8k, 16k, 22k, 44.1k
	 */
	unsigned int getSampleRate();

	/*
	 * @brief 	depending on sound quality vs size, you will likely want 8 or 16bit depth
	 * @return	bits per sample
	 */
	unsigned short getBitsPerSample();

	/*
	 * @brief get next sample
	 *
	 * @param	sample : pointer to 32bit sample storage
	 *
	 * @return
	 * 	- false: end of WAV is reached
	 * 	- true: success
	 */
	int nextSample(int* sample);

	/*
	 * @brief rewind the iterator to the beginning of the wav
	 */
	void rewind(void);

	/*
	 * @brief	is wav data initialized
	 */
	bool isInitialized() { return initialized; }

private:
	unsigned int wavpointer = 0;
	unsigned int wavdatapointer = 0;
	unsigned int wavdatasize = 0;

	uint16_t audioSample = 0;

	unsigned int audioSampleRate = 0;
	unsigned int bitsPerSample = 0;
	const char* wavdata = NULL;
	unsigned int wavsize = 0;
	bool initialized = false;

};

#endif /* MAIN_WAV_H_ */


