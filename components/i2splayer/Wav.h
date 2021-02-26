#include <stdint.h>
#include <stdio.h>

#ifndef MAIN_WAV_H_
#define MAIN_WAV_H_

class DataStream {

public:
	virtual bool Open() = 0;
	virtual bool Read(char* buf, unsigned int len) = 0;
	virtual void Close() = 0;

	virtual ~DataStream() {};
};

/*
 * @brief Wav class takes a pointer to a Wav file bytearray in memory or on flash program area.
 */
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
	bool init(DataStream* pDataStream);

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
	 * @param	sample : pointer to 32bit sample storage -- mono audio data sample will be written here
	 *
	 * @return
	 * 	- false: end of WAV is reached
	 * 	- true: success
	 */
	bool nextSample(int* sample);

	/*
	 * @brief rewind the iterator to the beginning of the wav
	 */
	//void rewind(void);

	/*
	 * @brief	is wav data initialized
	 */
	//bool isInitialized() { return mInitialized; }

	/*
	 * return   1 for mono
	 * 			2 for stereo
	 */
	unsigned short getChannelCount() { return mChannels; }

	/*
	 * return bytes of actual wav audio data, without headers
	 */
	unsigned int getPayloadSize() { return mWavdatasize; }

private:
	unsigned int mWavdatasize = 0;

	uint16_t mAudioSample = 0;

	unsigned short mChannels = 0;
	unsigned int mAudioSampleRate = 0;
	unsigned int mBitsPerSample = 0;
	//bool mInitialized = false;

	DataStream* mpDataStream = NULL;

};

#endif /* MAIN_WAV_H_ */


