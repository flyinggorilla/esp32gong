#include "Wav.h"
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


Wav::Wav() {


}

Wav::~Wav()  {

}


bool Wav::init(DataStream* pDataStream) {
  mpDataStream = pDataStream;
  if (!mpDataStream)
    return false;

  // WAV format spec: http://soundfile.sapp.org/doc/WaveFormat/

  //this->wavdata = wavdata;
// this->wavsize = size;

  //ESP_LOGI(LOGTAG, "WAV size: %i", wavsize);
  uint32_t uint32buf;
  uint16_t uint16buf;

  // read ChunkID "RIFF" header
  mpDataStream->Read((char*)&uint32buf, sizeof(uint32_t));
  if (uint32buf != 0x46464952) {
    ESP_LOGE(LOGTAG, "No RIFF format header %08X", uint32buf);
    return false;
  }

  // read Chunksize: remaining file size
  mpDataStream->Read((char*)&uint32buf, sizeof(uint32_t));
  ESP_LOGI(LOGTAG, "Chunksize from here: %u", uint32buf);

  // read Format header: "WAVE"
  mpDataStream->Read((char*)&uint32buf, sizeof(uint32_t));
  if (uint32buf != 0x45564157 ) {
      ESP_LOGE(LOGTAG, "No WAV format header ");
    return false;
  }

  // read Subchunk1ID: Format header: "fmt "
  mpDataStream->Read((char*)&uint32buf, sizeof(uint32_t));
  if (uint32buf != 0x20746d66  ) {
    ESP_LOGE(LOGTAG, "No 'fmt' format header ");
    return false;
  }

  // read subChunk1size
  mpDataStream->Read((char*)&uint32buf, sizeof(uint32_t));
  ESP_LOGI(LOGTAG, "SubChunk1size: %u", uint32buf);


  // read AudioFormat
  mpDataStream->Read((char*)&uint16buf, sizeof(uint16_t));
  if (uint16buf != 1  ) {
    ESP_LOGE(LOGTAG, "Invalid audio format ");
    return false;
  }

  // read NumChannels --- accept only mono files for now
  mpDataStream->Read((char*)&uint16buf, sizeof(uint16_t));
  mChannels = uint16buf;
  if (mChannels != 1  ) {
    ESP_LOGE(LOGTAG, "Too many channels. Only MONO files accepted. No Stereo.");
    return false;
  }

  // read sample rate
  mpDataStream->Read((char*)&uint32buf, sizeof(uint32_t));
  mAudioSampleRate = uint32buf;
  ESP_LOGI(LOGTAG, "Sample rate: %u", mAudioSampleRate);


  // read byte rate
  mpDataStream->Read((char*)&uint32buf, sizeof(uint32_t));
  ESP_LOGI(LOGTAG, "Byte rate: %u", uint32buf);

  // read BlockAlign
  mpDataStream->Read((char*)&uint16buf, sizeof(uint16_t));
  ESP_LOGI(LOGTAG, "Block align: %u", uint16buf);

  // read BitsPerSample
  mpDataStream->Read((char*)&uint16buf, sizeof(uint16_t));
  mBitsPerSample = uint16buf;
  ESP_LOGI(LOGTAG, "Bits per sample: %u bits", mBitsPerSample);

  // read Subchunk2ID: Format header: "data"
  mpDataStream->Read((char*)&uint32buf, sizeof(uint32_t));
  if (uint32buf != 0x61746164   ) {
    ESP_LOGE(LOGTAG, "No Data header 0x61746164 (instead: %08X)", uint32buf);
    return false;
  }

  // read subChunk1size
  mpDataStream->Read((char*)&uint32buf, sizeof(uint32_t));
  mWavdatasize = uint32buf;
  ESP_LOGI(LOGTAG, "data size / subChunk2size: : %u", mWavdatasize);

  // remember this location and skip header reading?
  //mWavdatapointer = wavpointer;

  //mInitialized = true;

  return true;
}

/*
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

    // read NumChannels --- accept only mono files for now
    uint16buf = (uint16_t*)&wavdata[wavpointer];
    wavpointer+=sizeof(uint16_t);
    channels = *uint16buf;
    if (channels != 1  ) {
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
   //wavpointer = wavdatapointer;
}
*/

unsigned int Wav::getSampleRate() {
	return mAudioSampleRate;
}

unsigned short Wav::getBitsPerSample() {
	return mBitsPerSample;
}


bool Wav::nextSample(int* pSample) {

	if (mBitsPerSample == 8) {
    uint8_t s;
    if (!mpDataStream->Read((char*)&s, 1)) {
      return false;
    }
    *pSample = s;
		*pSample -= 127;
		*pSample = *pSample << 8;
	} else if (mBitsPerSample == 16) {
    uint16_t s;
    if (!mpDataStream->Read((char*)&s, 2)) {
      return false;
    }
		*pSample = s;
	} else {
	    ESP_LOGE(LOGTAG, "Bit-depth not supported: %u", mBitsPerSample);
		return false;
	}
	return true;
}

/*
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
}*/

