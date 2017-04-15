#include "Wav.hpp"

#include "esp_log.h"
#define LOGTAG "wav"





//format bytes
/*String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}*/



/*
 * @brief Wav class takes a pointer to a Wav file bytearray in memory or on flash.
 *
 * @param const char wavdata[] input data
 * @param size of wavdata[] array
 */
Wav::Wav() {


}

Wav::~Wav()  {

}




bool Wav::init(const char* wavdata, unsigned int size) {
    // WAV format spec: http://soundfile.sapp.org/doc/WaveFormat/
	this->wavdata = wavdata;
	this->wavsize = size;

    ESP_LOGI(LOGTAG, "WAV size: %i", wavsize);


    // read ChunkID "RIFF" header
    uint32_t* uint32buf = (uint32_t*)&wavdata[wavpointer];
    wavpointer+=sizeof(uint32_t);
    if (*uint32buf != 0x46464952) {
      ESP_LOGE(LOGTAG, "No RIFF format header %08X", *uint32buf);
      return false;
    }

    // read Chunksize: remaining file size
    uint32buf = (uint32_t*)&wavdata[wavpointer];
    wavpointer+=sizeof(uint32_t);
    ESP_LOGI(LOGTAG, "Chunksize from here: %u", *uint32buf);

    // read Format header: "WAVE"
    uint32buf = (uint32_t*)&wavdata[wavpointer];
    wavpointer+=sizeof(uint32_t);
    if (*uint32buf != 0x45564157 ) {
        ESP_LOGE(LOGTAG, "No WAV format header ");
      return false;
    }

    // read Subchunk1ID: Format header: "fmt "
    uint32buf = (uint32_t*)&wavdata[wavpointer];
    wavpointer+=sizeof(uint32_t);
    if (*uint32buf != 0x20746d66  ) {
      ESP_LOGE(LOGTAG, "No 'fmt' format header ");
      return false;
    }

    // read subChunk1size
    uint32buf = (uint32_t*)&wavdata[wavpointer];
    wavpointer+=sizeof(uint32_t);
    ESP_LOGI(LOGTAG, "SubChunk1size: %u", *uint32buf);


    // read AudioFormat
    uint16_t* uint16buf = (uint16_t*)&wavdata[wavpointer];
    wavpointer+=sizeof(uint16_t);
    if (*uint16buf != 1  ) {
      ESP_LOGE(LOGTAG, "Invalid audio format ");
      return false;
    }

    // read NumChannels
    uint16buf = (uint16_t*)&wavdata[wavpointer];
    wavpointer+=sizeof(uint16_t);
    if (*uint16buf != 1  ) {
      ESP_LOGE(LOGTAG, "Too many channels. Only MONO files accepted. No Stereo.");
      return false;
    }

    // read sample rate
    uint32buf = (uint32_t*)&wavdata[wavpointer];
    wavpointer+=sizeof(uint32_t);
    audioSampleRate = *uint32buf;
    ESP_LOGI(LOGTAG, "Sample rate: %u", audioSampleRate);


    // read byte rate
    uint32buf = (uint32_t*)&wavdata[wavpointer];
    wavpointer+=sizeof(uint32_t);
    ESP_LOGI(LOGTAG, "Byte rate: %u", *uint32buf);

   // read BlockAlign
    uint16buf = (uint16_t*)&wavdata[wavpointer];
    wavpointer+=sizeof(uint16_t);
    ESP_LOGI(LOGTAG, "Block align: %u", *uint16buf);

   // read BitsPerSample
    uint16buf = (uint16_t*)&wavdata[wavpointer];
    wavpointer+=sizeof(uint16_t);
    bitsPerSample = *uint16buf;
    ESP_LOGI(LOGTAG, "Bits per sample: %u bits", bitsPerSample);

    // read Subchunk2ID: Format header: "data"
    uint32buf = (uint32_t*)&wavdata[wavpointer];
    wavpointer+=sizeof(uint32_t);
    if (*uint32buf != 0x61746164   ) {
      ESP_LOGE(LOGTAG, "No Data header 0x61746164 (instead: %08X)", *uint32buf);
      return false;
    }

    // read subChunk1size
    uint32buf = (uint32_t*)&wavdata[wavpointer];
    wavpointer+=sizeof(uint32_t);
    wavdatasize = *uint32buf;
    ESP_LOGI(LOGTAG, "data size / subChunk2size: : %u", wavdatasize);

    wavdatapointer = wavpointer;

    initialized = true;

    return true;
}


void Wav::rewind(void) {
   wavpointer = wavdatapointer;
}

unsigned int Wav::getSampleRate() {
	return audioSampleRate;
}

unsigned short Wav::getBitsPerSample() {
	return bitsPerSample;
}


int Wav::nextSample(int* sample) {
	if (wavpointer >= wavdatapointer + wavdatasize - bitsPerSample/8) {
	    //ESP_LOGI(LOGTAG, "End of WAV reached");
		return false;
	}

	if (bitsPerSample == 8) {
		*sample = (uint8_t)wavdata[wavpointer];
		*sample -= 127;
		*sample = *sample << 8;
		wavpointer += sizeof(uint8_t);
	} else if (bitsPerSample == 16) {
		*sample = (int16_t)wavdata[wavpointer];
		wavpointer += sizeof(int16_t);
	} else {
	    ESP_LOGE(LOGTAG, "Bit-depth not supported: %u", bitsPerSample);
		return false;
	}
	return true;
}

