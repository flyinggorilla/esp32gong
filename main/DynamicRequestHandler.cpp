#include "DynamicRequestHandler.h"
#include "Esp32Gong.h"
#include "Config.h"
#include "Storage.h"
#include "I2SPlayer.h"
#include "esp_system.h"
#include "esp_log.h"
#include "Ota.h"
#include "EspString.h"
#include "WebClient.h"
#include "Wifi.h"
#include "temperature.h"
#include "MemoryDataStream.h"
#include "StorageDataStream.h"

extern const unsigned char gong_wav_start[] asm("_binary_gong_wav_start");
extern const unsigned char gong_wav_end[]   asm("_binary_gong_wav_end");
unsigned int uiGongWavLength = gong_wav_end - gong_wav_start;
const unsigned char* bGongWav = gong_wav_start;



static char tag[] = "DynamicRequestHandler";

extern Esp32Gong esp32gong;
extern I2SPlayer musicPlayer;
extern Wifi wifi;
extern Storage storage;

#define OTA_LATEST_FIRMWARE_JSON_URL "http://surpro5:9999/version.json" // testing with local go server
#define OTA_LATEST_FIRMWARE_URL "http://surpro5:9999/esp32gong.bin"		// testing with local go server
//#define OTA_LATEST_FIRMWARE_JSON_URL "https://raw.githubusercontent.com/Dynatrace/ufo-esp32/master/firmware/version.json"
//#define OTA_LATEST_FIRMWARE_URL "https://raw.githubusercontent.com/Dynatrace/ufo-esp32/master/firmware/ufo-esp32.bin"

DynamicRequestHandler::DynamicRequestHandler()
{
	mbRestart = false;
}

DynamicRequestHandler::~DynamicRequestHandler()
{
}

bool DynamicRequestHandler::HandleApiRequest(std::list<TParam> &params, HttpResponse &rResponse)
{

	esp32gong.IndicateApiCall();

	String sBody;
	std::list<TParam>::iterator it = params.begin();

	int volume = 100;
	bool play = false;
	String file;
	while (it != params.end())
	{

		if ((*it).paramName == "gong")
		{
			play = true;
		}
		else if ((*it).paramName == "volume")
		{
			long volumeParam = (*it).paramValue.toInt();
			if ((volumeParam >= 0) && (volumeParam <= 100))
			{
				volume = volumeParam;
				ESP_LOGI(tag, "gong playing with volume %i", volume);
			}
			else
			{
				ESP_LOGW(tag, "invalid volume - valid value range is from 0 (off) to 100 (max)");
			}
		}
		else if ((*it).paramName == "file")
		{
			file = (*it).paramValue;
			ESP_LOGI(tag, "gong playing file %s", file.c_str());
		}
		it++;
	}
	if (play)
	{
		musicPlayer.setVolume(volume);
		if (file.length()) {
			musicPlayer.playAsync(new StorageDataStream(file));
		} else {
			ESP_LOGW(tag, "No WAV file to play");
			musicPlayer.playAsync(new MemoryDataStream(bGongWav, uiGongWavLength));
		}
		sBody = "<html><body>api call - lets play music</html></body>";
	}

	rResponse.AddHeader(HttpResponse::HeaderNoCache);
	rResponse.SetRetCode(200);
	return rResponse.Send(sBody.c_str(), sBody.length());
}

bool DynamicRequestHandler::HandleInfoRequest(std::list<TParam> &params, HttpResponse &rResponse)
{

	char sHelp[20];
	String sBody;

	sBody.reserve(512);
	sBody.printf("{\"apmode\":\"%s\",", esp32gong.GetConfig().mbAPMode ? "1" : "0");
	sBody.printf("\"heap\":\"%d\",", esp_get_free_heap_size());
	sBody.printf("\"ssid\":\"%s\",", esp32gong.GetConfig().msSTASsid.c_str());
	sBody.printf("\"hostname\":\"%s\",", esp32gong.GetConfig().msHostname.c_str());
	sBody.printf("\"enterpriseuser\":\"%s\",", esp32gong.GetConfig().msSTAENTUser.c_str());
	sBody.printf("\"sslenabled\":\"%s\",", esp32gong.GetConfig().mbWebServerUseSsl ? "1" : "0");
	sBody.printf("\"listenport\":\"%d\",", esp32gong.GetConfig().muWebServerPort);

	if (esp32gong.GetConfig().mbAPMode)
	{
		sBody.printf("\"lastiptoap\":\"%d.%d.%d.%d\",", IP2STR((ip4_addr *)&(esp32gong.GetConfig().muLastSTAIpAddress)));
	}
	else
	{
		sBody += "\"ipaddress\":\"";
		sBody += wifi.GetLocalAddress();
		wifi.GetGWAddress(sHelp);
		sBody.printf("\",\"ipgateway\":\"%s\",", sHelp);
		wifi.GetNetmask(sHelp);
		sBody.printf("\"ipsubnetmask\":\"%s\",", sHelp);

		uint8_t uChannel;
		int8_t iRssi;
		wifi.GetApInfo(iRssi, uChannel);
		sBody.printf("\"rssi\":\"%d\",", iRssi);
		sBody.printf("\"channel\":\"%d\",", uChannel);
	}
	wifi.GetMac((__uint8_t *)sHelp);

	sBody.printf("\"macaddress\":\"%x:%x:%x:%x:%x:%x\",", sHelp[0], sHelp[1], sHelp[2], sHelp[3], sHelp[4], sHelp[5]);
	sBody.printf("\"firmwareversion\":\"%s\",", FIRMWARE_VERSION);
	sBody.printf("\"temperature\":\"%3.1f\",", esp32_temperature());
	//sBody.printf("\"deviceid\":\"%s\",", esp32gong.GetConfig().msUfoId.c_str());
	//sBody.printf("\"devicename\":\"%s\",", esp32gong.GetConfig().msUfoName.c_str());
	sBody.printf("\"organization\":\"%s\",", esp32gong.GetConfig().msOrganization.c_str());
	sBody.printf("\"department\":\"%s\",", esp32gong.GetConfig().msDepartment.c_str());
	sBody.printf("\"location\":\"%s\"", esp32gong.GetConfig().msLocation.c_str());
	sBody += '}';

	rResponse.AddHeader(HttpResponse::HeaderContentTypeJson);
	rResponse.AddHeader(HttpResponse::HeaderNoCache);
	rResponse.SetRetCode(200);
	return rResponse.Send(sBody.c_str(), sBody.length());
}

bool DynamicRequestHandler::HandleStorageRequest(std::list<TParam> &params, HttpResponse &rResponse)
{

	String sBody;

	std::list<TParam>::iterator it = params.begin();

	bool bInfo = false;
	bool bDelete = false;
	while (it != params.end())
	{

		if ((*it).paramName == "info")
		{
			ESP_LOGD(tag, "requesting storage info");
			bInfo = true;
			break;
		}
		else if ((*it).paramName == "deletefile")
		{
			ESP_LOGD(tag, "deleting file %s", (*it).paramValue.c_str());
			bDelete = true;
			break;
		}
		it++;
	}

	if (bInfo)
	{
		sBody.reserve(512);
		sBody.printf("{\"freebytes\":\"%u\",", storage.FreeBytes());
		sBody.printf("\"totalbytes\":\"%u\",", storage.TotalBytes());
		sBody.printf("\"files\": [ ");

		std::list<TDirEntry> dir;
		storage.ListDirectory(dir);
		std::list<TDirEntry>::iterator dit = dir.begin();

		while (dit != dir.end())
		{
			if (dit != dir.begin())
			{
				sBody += ", ";
			}
			sBody.printf("{\"filename\":\"%s\", \"bytes\":\"%li\"}", (*dit).name.c_str(), (*dit).size);
			dit++;
		}

		sBody += "]}";
		rResponse.AddHeader(HttpResponse::HeaderContentTypeJson);
		rResponse.AddHeader(HttpResponse::HeaderNoCache);
	}
	else if (bDelete)
	{
		if (storage.Delete((*it).paramValue))
		{
			sBody = "<html><head><title>SUCCESS - file deletion succeeded</title>"
					"<meta http-equiv=\"refresh\" content=\"2; url=/#!pagestorage\"></head><body>"
					"<h2>SUCCESS - file deletion succeeded, returning to page shortly.</h2></body></html>";
		}
		else
		{
			sBody = "<html><head><title>ERROR - could not delete file</title>"
					"<meta http-equiv=\"refresh\" content=\"2; url=/#!pagestorage\"></head><body>"
					"<h2>ERROR - file deletion failed, returning to page shortly.</h2></body></html>";
		}
	}
	else
	{
		sBody = "{ }";
	}

	rResponse.SetRetCode(200);
	return rResponse.Send(sBody.c_str(), sBody.length());
}

bool DynamicRequestHandler::HandleConfigRequest(std::list<TParam> &params, HttpResponse &rResponse)
{

	const char *sWifiMode = NULL;
	const char *sWifiSsid = NULL;
	const char *sWifiPass = NULL;
	const char *sWifiEntPass = NULL;
	const char *sWifiEntUser = NULL;
	const char *sWifiEntCA = NULL;
	const char *sWifiHostName = NULL;

	String sBody;
	ESP_LOGI(tag, "HANDLING CONFIG REQUEST: <<<<< %s >>>>>", sBody.c_str());

	std::list<TParam>::iterator it = params.begin();
	while (it != params.end())
	{

		if ((*it).paramName == "wifimode")
			sWifiMode = (*it).paramValue.c_str();
		else if ((*it).paramName == "wifissid")
		{
			sWifiSsid = (*it).paramValue.c_str();
			ESP_LOGI(tag, "SETTING SSID TO: %s", sWifiSsid);
		}
		else if ((*it).paramName == "wifipwd")
			sWifiPass = (*it).paramValue.c_str();
		else if ((*it).paramName == "wifientpwd")
			sWifiEntPass = (*it).paramValue.c_str();
		else if ((*it).paramName == "wifientuser")
			sWifiEntUser = (*it).paramValue.c_str();
		else if ((*it).paramName == "wifientca")
			sWifiEntCA = (*it).paramValue.c_str();
		else if ((*it).paramName == "wifihostname")
		{
			sWifiHostName = (*it).paramValue.c_str();
			ESP_LOGI(tag, "SETTING HOSTNAME TO: %s", sWifiHostName);
		}
		it++;
	}

	bool bOk = false;
	if (sWifiSsid)
	{
		ESP_LOGI(tag, "YES SSID %s IS SET -- activating Station mode", sWifiSsid);

		esp32gong.GetConfig().msSTASsid = sWifiSsid;

		if (sWifiMode && (sWifiMode[0] == '2'))
		{ //enterprise wap2
			if (sWifiEntUser && (sWifiEntUser[0] != 0x00))
			{
				esp32gong.GetConfig().msSTAENTUser = sWifiEntUser;
				if (sWifiEntCA)
					esp32gong.GetConfig().msSTAENTCA = sWifiEntCA;
				else
					esp32gong.GetConfig().msSTAENTCA.clear();
				if (sWifiEntPass)
					esp32gong.GetConfig().msSTAPass = sWifiEntPass;
				else
					esp32gong.GetConfig().msSTAPass.clear();
				bOk = true;
			}
		}
		else
		{
			ESP_LOGI(tag, "Configuring Wifi Station");
			if (sWifiPass)
				esp32gong.GetConfig().msSTAPass = sWifiPass;
			else
				esp32gong.GetConfig().msSTAPass.clear();
			esp32gong.GetConfig().msSTAENTUser.clear();
			esp32gong.GetConfig().msSTAENTCA.clear();
			bOk = true;
		}
	}
	if (sWifiHostName && !esp32gong.GetConfig().msHostname.equals(sWifiHostName))
	{
		ESP_LOGI(tag, "Setting new hostname");

		esp32gong.GetConfig().msHostname = sWifiHostName;
		bOk = true;
	}
	if (bOk)
	{
		ESP_LOGI(tag, "FINALLY turning off AP mode and writing config");

		esp32gong.GetConfig().mbAPMode = false;
		esp32gong.GetConfig().Write();
		mbRestart = true;
		sBody = "<html><head><title>SUCCESS - firmware update succeded, rebooting shortly.</title>"
				"<meta http-equiv=\"refresh\" content=\"10; url=/\"></head><body>"
				"<h2>New settings stored, rebooting shortly.</h2></body></html>";
		rResponse.SetRetCode(200);
	}
	else
	{
		rResponse.AddHeader("Location: /#!pagewifisettings");
		rResponse.SetRetCode(302);
	}

	rResponse.AddHeader(HttpResponse::HeaderNoCache);
	return rResponse.Send(sBody.c_str(), sBody.length());
}

bool DynamicRequestHandler::HandleSrvConfigRequest(std::list<TParam> &params, HttpResponse &rResponse)
{
	const char *sSslEnabled = NULL;
	const char *sListenPort = NULL;
	const char *sServerCert = NULL;
	const char *sCurrentHost = NULL;

	String sBody;

	std::list<TParam>::iterator it = params.begin();
	while (it != params.end())
	{
		if ((*it).paramName == "sslenabled")
			sSslEnabled = (*it).paramValue.c_str();
		else if ((*it).paramName == "listenport")
			sListenPort = (*it).paramValue.c_str();
		else if ((*it).paramName == "servercert")
			sServerCert = (*it).paramValue.c_str();
		else if ((*it).paramName == "currenthost")
			sCurrentHost = (*it).paramValue.c_str();
		it++;
	}
	esp32gong.GetConfig().mbWebServerUseSsl = (sSslEnabled != NULL);
	esp32gong.GetConfig().muWebServerPort = atoi(sListenPort);
	esp32gong.GetConfig().msWebServerCert = sServerCert;
	ESP_LOGD(tag, "HandleSrvConfigRequest %d, %d", esp32gong.GetConfig().mbWebServerUseSsl, esp32gong.GetConfig().muWebServerPort);
	esp32gong.GetConfig().Write();
	mbRestart = true;

	String newUrl = "/";
	if (sCurrentHost)
	{
		String sHost = sCurrentHost;
		int i = sHost.indexOf(':');
		if (i >= 0)
			sHost = sHost.substring(0, i);
		if (sHost.length())
		{
			if (sSslEnabled != NULL)
			{
				newUrl = "https://" + sHost;
				if (esp32gong.GetConfig().muWebServerPort && (esp32gong.GetConfig().muWebServerPort != 443))
				{
					newUrl += ':';
					newUrl += sListenPort;
				}
			}
			else
			{
				newUrl = "http://" + sHost;
				if (esp32gong.GetConfig().muWebServerPort && (esp32gong.GetConfig().muWebServerPort != 80))
				{
					newUrl += ':';
					newUrl += sListenPort;
				}
			}
			newUrl += '/';
		}
	}

	sBody = "<html><head><title>SUCCESS - firmware update succeded, rebooting shortly.</title>"
			"<meta http-equiv=\"refresh\" content=\"10; url=";
	sBody += newUrl;
	sBody += "\"></head><body><h2>New settings stored, rebooting shortly.</h2></body></html>";
	rResponse.SetRetCode(200);
	rResponse.AddHeader(HttpResponse::HeaderNoCache);

	return rResponse.Send(sBody);
}

/*
GET: /firmware?update
GET: /firmware?progress
Response:
{ "session": "9724987887789", 
"progress": "22",
"status": "inprogress" }
Session: 32bit unsigned int ID that changes when UFO reboots
Progress: 0..100%
Status: notyetstarted | inprogress | connectionerror | flasherror | finishedsuccess
notyetstarted: Firmware update process has not started.
inprogress: Firmware update is in progress.
connectionerror: Firmware could not be downloaded. 
flasherror: Firmware could not be flashed.
finishedsuccess: Firmware successfully updated. Rebooting now.
*/

bool DynamicRequestHandler::HandleFirmwareRequest(std::list<TParam> &params, HttpResponse &response)
{
	std::list<TParam>::iterator it = params.begin();
	String sBody;
	response.SetRetCode(400); // invalid request
	while (it != params.end())
	{

		if ((*it).paramName == "progress")
		{
			short progressPct = 0;
			const char *progressStatus = "notyetstarted";
			int progress = Ota::GetProgress();
			if (progress >= 0)
			{
				progressPct = progress;
				progressStatus = "inprogress";
			}
			else
			{
				switch (progress)
				{
				case OTA_PROGRESS_NOTYETSTARTED:
					progressStatus = "notyetstarted";
					break;
				case OTA_PROGRESS_CONNECTIONERROR:
					progressStatus = "connectionerror";
					break;
				case OTA_PROGRESS_FLASHERROR:
					progressStatus = "flasherror";
					break;
				case OTA_PROGRESS_FINISHEDSUCCESS:
					progressStatus = "finishedsuccess";
					progressPct = 100;
					break;
				}
			}
			sBody = "{ \"session\": \"";
			sBody += Ota::GetTimestamp();
			sBody += "\", \"progress\": \"";
			sBody += progressPct;
			sBody += "\", \"status\": \"";
			sBody += progressStatus;
			sBody += "\"}";
			response.AddHeader(HttpResponse::HeaderContentTypeJson);
			response.SetRetCode(200);
		}
		else if ((*it).paramName == "update")
		{
			if (Ota::GetProgress() == OTA_PROGRESS_NOTYETSTARTED)
			{
				Ota::StartUpdateFirmwareTask(OTA_LATEST_FIRMWARE_URL);
				//TODO implement firmware version check;
			}
			// {"status":"firmware update initiated.", "url":"https://github.com/Dynatrace/ufo-esp32/raw/master/firmware/ufo-esp32.bin"}
			sBody = "{\"status\":\"firmware update initiated.\", \"url\":\"";
			sBody += OTA_LATEST_FIRMWARE_URL;
			sBody += "\"}";
			response.AddHeader(HttpResponse::HeaderContentTypeJson);
			response.SetRetCode(200);
		}
		else if ((*it).paramName == "check")
		{
			//TODO implement firmware version check;
			sBody = "not implemented";
			response.SetRetCode(501); // not implemented
		}
		else if ((*it).paramName == "restart")
		{
			//TODO implement firmware version check;
			sBody = "restarting...";
			mbRestart = true;
			response.SetRetCode(200);
		}
		else if ((*it).paramName == "switchbootpartition")
		{
			Ota ota;
			if (ota.SwitchBootPartition())
			{
				mbRestart = true;
				sBody = "Switching boot partition successful.";
				response.SetRetCode(200);
			}
			else
			{
				//TODO add ota.GetErrorInfo() to inform end-user of problem
				sBody = "Switching boot partition failed.";
				response.SetRetCode(500);
			}
		}
		else
		{
			sBody = "Invalid request.";
			response.SetRetCode(400);
		}
		it++;
	}
	response.AddHeader(HttpResponse::HeaderNoCache);
	return response.Send(sBody.c_str(), sBody.length());
}

bool DynamicRequestHandler::HandleCheckFirmwareRequest(std::list<TParam> &params, HttpResponse &response)
{

	String sBody;
	response.SetRetCode(404); // not found

	Url url;
	url.Parse(OTA_LATEST_FIRMWARE_JSON_URL);

	ESP_LOGD(tag, "Retrieve json from: %s", url.GetUrl().c_str());
	WebClient webClient;
	webClient.Prepare(&url);

	unsigned short statuscode = webClient.HttpGet();
	if (statuscode != 200)
		return false;
	int i = webClient.GetResponseData().indexOf("\"version\":\"");
	if (i <= 0)
		return false;
	String version = webClient.GetResponseData().substring(i + 11);
	i = version.indexOf('"');
	if (i <= 0)
		return false;
	version = version.substring(0, i);

	if (!version.equalsIgnoreCase(FIRMWARE_VERSION))
	{
		sBody = "{\"newversion\":\"Firmware available: ";
		sBody += version;
		sBody += "\"}";
	}
	else
		sBody = "{}";
	response.SetRetCode(200);
	return response.Send(sBody);
}