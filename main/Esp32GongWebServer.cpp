#include "Esp32GongWebServer.h"
#include "Esp32Gong.h"
#include "config.h"
#include "DynamicRequestHandler.h"
#include "HttpRequestParser.h"
#include "HttpResponse.h"
#include <lwip/sockets.h>
#include <esp_log.h>
#include <esp_system.h>
#include <String.h>

#include "sdkconfig.h"
#include "indexhtml.h"
#include "Storage.h"

extern Esp32Gong esp32gong;
extern Storage storage;

static char tag[] = "Esp32GongWebServer";

extern const unsigned char font_ttf_start[] asm("_binary_material_design_icons_ttf_start");
extern const unsigned char font_ttf_end[]   asm("_binary_material_design_icons_ttf_end");
unsigned int uiFontTtfLength = font_ttf_end - font_ttf_start;
const unsigned char* bFontTtf = font_ttf_start;

extern const unsigned char font_eot_start[] asm("_binary_material_design_icons_eot_start");
extern const unsigned char font_eot_end[]   asm("_binary_material_design_icons_eot_end");
unsigned int uiFontEotLength = font_eot_end - font_eot_start;
const unsigned char* bFontEot = font_eot_start;

extern const unsigned char font_svg_start[] asm("_binary_material_design_icons_svg_start");
extern const unsigned char font_svg_end[]   asm("_binary_material_design_icons_svg_end");
unsigned int uiFontSvgLength = font_svg_end - font_svg_start;
const unsigned char* bFontSvg = font_svg_start;

extern const unsigned char font_woff_start[] asm("_binary_material_design_icons_woff_start");
extern const unsigned char font_woff_end[]   asm("_binary_material_design_icons_woff_end");
unsigned int uiFontWoffLength = font_woff_end - font_woff_start;
const unsigned char* bFontWoff = font_woff_start;


//------------------------------------------------------------------

Esp32GongWebServer::Esp32GongWebServer()
{
	mbRestart = false;
}

Esp32GongWebServer::~Esp32GongWebServer()
{
}

bool Esp32GongWebServer::StartWebServer()
{

	AddUploadHandler("/update", &mOta);
	AddUploadHandler("/fileupload", &storage);

	if (esp32gong.GetConfig().mbAPMode)
		return Start(80, false, NULL);

	__uint16_t port;
	if (esp32gong.GetConfig().muWebServerPort)
		port = esp32gong.GetConfig().muWebServerPort;
	else
		port = esp32gong.GetConfig().mbWebServerUseSsl ? 443 : 80;

	return Start(port, esp32gong.GetConfig().mbWebServerUseSsl, &(esp32gong.GetConfig().msWebServerCert));
}

bool Esp32GongWebServer::HandleRequest(HttpRequestParser &httpParser, HttpResponse &httpResponse)
{

	DynamicRequestHandler requestHandler;

	ESP_LOGI(tag, "TTF Length %d", uiFontTtfLength);

	if (httpParser.GetUrl().equals("/") || httpParser.GetUrl().equals("/index.html"))
	{
		httpResponse.AddHeader(HttpResponse::HeaderContentTypeHtml);
		httpResponse.AddHeader("Content-Encoding: gzip");
		if (!httpResponse.Send(indexhtml_h, sizeof(indexhtml_h)))
			return false;
	}
	else if (httpParser.GetUrl().equals("/fonts/material-design-icons.woff"))
	{
		httpResponse.AddHeader(HttpResponse::HeaderContentTypeBinary);
		//if (!httpResponse.Send(fontwoff_h, sizeof(fontwoff_h)))
		if (!httpResponse.Send((const char*)bFontWoff, uiFontWoffLength))
			return false;
	}
	else if (httpParser.GetUrl().equals("/fonts/material-design-icons.ttf"))
	{
		httpResponse.AddHeader(HttpResponse::HeaderContentTypeBinary);
		//if (!httpResponse.Send(fontttf_h, sizeof(fontttf_h)))
		if (!httpResponse.Send((const char*)bFontTtf, uiFontTtfLength))
			return false;
	}
	else if (httpParser.GetUrl().equals("/fonts/material-design-icons.eot"))
	{
		httpResponse.AddHeader(HttpResponse::HeaderContentTypeBinary);
		if (!httpResponse.Send((const char*)bFontEot, uiFontEotLength))
			return false;
	}
	else if (httpParser.GetUrl().equals("/fonts/material-design-icons.svg"))
	{
		httpResponse.AddHeader(HttpResponse::HeaderContentTypeBinary);
		if (!httpResponse.Send((const char*)bFontSvg, uiFontSvgLength))
			return false;
	}
	else if (httpParser.GetUrl().equals("/api"))
	{
		if (!requestHandler.HandleApiRequest(httpParser.GetParams(), httpResponse))
			return false;
	}
	else if (httpParser.GetUrl().equals("/info"))
	{
		if (!requestHandler.HandleInfoRequest(httpParser.GetParams(), httpResponse))
			return false;
	}
	else if (httpParser.GetUrl().equals("/config"))
	{
		if (!requestHandler.HandleConfigRequest(httpParser.GetParams(), httpResponse))
			return false;
	}
	else if (httpParser.GetUrl().equals("/storage"))
	{
		if (!requestHandler.HandleStorageRequest(httpParser.GetParams(), httpResponse))
			return false;
	}
	else if (httpParser.GetUrl().equals("/srvconfig"))
	{
		if (!requestHandler.HandleSrvConfigRequest(httpParser.GetParams(), httpResponse))
			return false;
	}
	else if (httpParser.GetUrl().equals("/firmware"))
	{
		if (!requestHandler.HandleFirmwareRequest(httpParser.GetParams(), httpResponse))
			return false;
	}
	else if (httpParser.GetUrl().equals("/checkfirmware"))
	{
		if (!requestHandler.HandleCheckFirmwareRequest(httpParser.GetParams(), httpResponse))
			return false;
	}
	else if (httpParser.GetUrl().equals("/update"))
	{
		String sBody = "<html><head><title>SUCCESS - firmware update succeeded, rebooting shortly.</title>"
					   "<meta http-equiv=\"refresh\" content=\"10; url=/\"></head><body>"
					   "<h2>SUCCESS - firmware update succeeded, rebooting shortly.</h2></body></html>";
		if (!httpResponse.Send(sBody))
			return false;
	}

	else if (httpParser.GetUrl().equals("/test"))
	{
		String sBody;
		sBody = httpParser.IsGet() ? "GET " : "POST ";
		sBody += httpParser.GetUrl();
		sBody += httpParser.IsHttp11() ? " HTTP/1.1" : "HTTP/1.0";
		sBody += "\r\n";
		std::list<TParam> params = httpParser.GetParams();
		std::list<TParam>::iterator it = params.begin();
		while (it != params.end())
		{
			sBody += (*it).paramName;
			sBody += " = ";
			sBody += (*it).paramValue;
			sBody += "\r\n";
			it++;
		}
		if (!httpParser.IsGet())
		{
			sBody += "Boundary:<";
			sBody += httpParser.GetBoundary();
			sBody += ">\r\n";
			sBody += "Body:\r\n";
			sBody += httpParser.GetBody();
		}
		if (!httpResponse.Send(sBody))
			return false;
	}

	else if (httpParser.GetUrl().equals("/fileupload"))
	{
		String sBody = "<html><head><title>SUCCESS - file upload succeeded</title>"
					   "<meta http-equiv=\"refresh\" content=\"2; url=/#!pagestorage\"></head><body>"
					   "<h2>SUCCESS - file upload succeeded, returning to page shortly.</h2></body></html>";
		if (!httpResponse.Send(sBody))
			return false;
	}
	else
	{
		httpResponse.SetRetCode(404);
		if (!httpResponse.Send(NULL, 0))
			return false;
	}

	if (mbRestart || requestHandler.ShouldRestart() || (mOta.GetProgress() == OTA_PROGRESS_FINISHEDSUCCESS))
	{
		ESP_LOGI(tag, "RESTARTING!");
		vTaskDelay(100);
		esp_restart();
	}

	return true;
}
