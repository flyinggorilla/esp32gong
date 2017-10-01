# esp32gong
ESP32 based Gong with embedded webserver, Audio/Sound player, OTA and more - e.g. for Sparkfun thing, Adafruit Huzzah32, and many others

## features
* Webserver with GET/POST, multipart-mime upload and TLS support 
* Webclient with TLS support
* Url/Querystring parser
* Captive webserver in Access Point mode (needs improvement for true captive capabilities)
* Responsive Web UI based on Phonon 1.4.5
* Data embedding (such as HTML, CSS, font files, audio...)
* WAV decoder
* Audio player to I2S devices (e.g. Adafruit MAX98357A) 
* C++, ESP-IDF
* Wifi AP/STA mode (GPIO0 button will toggle mode)
* Stores config in NVS

todo:
* Basic SPIFFS read/write access for storing uploaded files on flash on dedicated data partition
* mp3 support

## build

* Setup ESP-IDF toolchain according to [http://esp-idf.readthedocs.io/en/latest/](http://esp-idf.readthedocs.io/en/latest/)
* run `make menuconfig` and adjust serial port 
* change partition setting to custom and choose `partitions.csv`. the partition table `partitions.csv`setup assumes that the ESP32 has at least 4MB flash available, so that their is a remaining of 896MB flash for SPIFFS data area
*  

## hardware

* SparkFun thing
* [Adafruit MAX98357A](https://www.adafruit.com/product/3006)
* Standard 3" speaker - 4Ohm, 3W e.g. [https://www.adafruit.com/product/1314](https://www.adafruit.com/product/1314)
* 3D printed case - [download model from thingiverse](https://www.thingiverse.com/thing:2562145)

![gong speaker-box](gong.jpg)




![speaker-box with sparkfun thing](wiring.jpg)


