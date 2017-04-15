# esp32gong
ESP32 based Gong with embedded webserver, Audio/Sound player, OTA and more - e.g. for Sparkfun thing or Espressif DevKitC

## features
* C++, ESP-IDF
* Wifi AP/STA mode (GPIO0 button will toggle mode)
* Stores config in NVS
* Basic SPIFFS read/write access for storing uploaded files on flash on dedicated data partition
* Webserver & Webclient based on Mongoose
* Responsive Web UI based on Phonon 1.4.5
* Data embedding (such as HTML, CSS, font files, audio...)
* Url/Querystring parser based on Yuarel
* WAV decoder
* Audio player to I2S devices (e.g. Adafruit MAX98357A) 

todo:
* OTA
* TLS
* captive webpage
* mp3 support

## build

* Setup ESP-IDF toolchain according to [http://esp-idf.readthedocs.io/en/latest/](http://esp-idf.readthedocs.io/en/latest/)
* run `make menuconfig` and adjust serial port 
* the partition table `partitions.csv`setup assumes that the ESP32 has at least 4MB flash available, so that their is a remaining of 896MB flash for SPIFFS data area

## hardware

![gong speaker-box][gong.jpg]


![speaker-box with sparkfun thing][wiring.jpg]


* SparkFun thing
* Adafruit MAX98357A
* Standard 3" speaker
* 3D printed case - [Fusion 360 model](http://a360.co/2oedo4p)