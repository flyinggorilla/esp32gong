#include "WebClient.h"

#include "Url.h"
#include <esp_log.h>
#include "sdkconfig.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <list>
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <netdb.h>

#include "HttpResponseParser.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "mbedtls/platform.h"
#include "mbedtls/net.h"
//#include "mbedtls/esp_debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

#define DEFAULT_MAXRESPONSEDATASIZE 16*1024
#define RECEIVE_BUFFER_SIZE 2*1024

static const char LOGTAG[] = "WebClient";

WebClient::WebClient() {
  muMaxResponseDataSize = DEFAULT_MAXRESPONSEDATASIZE;
}

WebClient::~WebClient() {

}

bool WebClient::Prepare(Url* pUrl) {
	mlRequestHeaders.clear();
	if (!pUrl)
		return false;
	mpUrl = pUrl;

	return true;
}

void WebClient::Clear(){
	mlRequestHeaders.clear();
	mHttpResponseParser.Clear();
}


bool WebClient::AddHttpHeader(String& sHeader) {
	mlRequestHeaders.push_back(sHeader);
	return true;
}

bool WebClient::AddHttpHeaderCStr(const char* header) {
	mlRequestHeaders.push_back(header);
	return true;
}

void WebClient::SetDownloadHandler(DownAndUploadHandler* pDownloadHandler) {
	mpDownloadHandler = pDownloadHandler;
}

unsigned short WebClient::HttpPost(const char* data, unsigned int size) {
	if (!data) return 0;

	mpPostData = data;
	muPostDataSize = size;
	return HttpExecute();
}

unsigned short WebClient::HttpPost(String& sData) {
	return HttpPost(sData.c_str(), sData.length());
}

void WebClient::PrepareRequest(String& sRequest) {
	sRequest.reserve(512);
	sRequest = mpPostData ? "POST " : "GET ";
	sRequest += mpUrl->GetPath();
	if (mpUrl->GetQueryParams().size()) {
		sRequest += '?';
		sRequest += mpUrl->GetQuery();
	}
	sRequest += " HTTP/1.0\r\nHost: ";
	sRequest += mpUrl->GetHost();
	sRequest += "\r\n";

	for (std::list<String>::iterator it = mlRequestHeaders.begin(); it != mlRequestHeaders.end(); ++it) {
		sRequest += *it; //TODO *it or it????
		sRequest += "\r\n";
	}

	if (mpPostData) {
		ESP_LOGD(LOGTAG, "SETTING Content-Length to %u", muPostDataSize);
		char contentLength[64];
		sprintf(contentLength, "Content-Length: %u\r\n", muPostDataSize);
		sRequest += contentLength;
	}

	sRequest += "User-Agent: esp32webclient/1.0 esp32\r\n\r\n";
}

unsigned short WebClient::HttpGet() {
	mpPostData = NULL;
	muPostDataSize = 0;
	unsigned short statuscode;
	
	if (!mpUrl) return 1001;

	for (short redirects = 0; redirects < 5; redirects++) {
		statuscode = HttpExecute();
		if (statuscode != 302 && statuscode != 301) {
			ESP_LOGD(LOGTAG, "HttpExecute* finished with %u", mHttpResponseParser.GetStatusCode());
			return statuscode;
		}
		mpUrl->Parse(mHttpResponseParser.GetRedirectLocation());
		ESP_LOGD(LOGTAG, "Redirecting to: %s", mHttpResponseParser.GetRedirectLocation().c_str());
	}
	return 399; // max redirects exceeded

}


unsigned short WebClient::HttpExecute() {

	if (!mpUrl)
		return 1001;

	if (mpUrl->GetHost().length() == 0) {
		return 1002;
	}

	if (mpUrl->GetSecure()) {
		return HttpExecuteSecure();
	}

	struct addrinfo *res;
	char service[6];
	sprintf(service, "%i", mpUrl->GetPort());
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	int err = getaddrinfo(mpUrl->GetHost().c_str(), service, &hints, &res);

	if (err != 0 || res == NULL) {
		ESP_LOGE(LOGTAG, "DNS lookup failed err=%d res=%p", err, res);
		return 1003;
	}

	// Code to print the resolved IP.
	// Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code
	struct in_addr *addr = &((struct sockaddr_in *) res->ai_addr)->sin_addr;
	ESP_LOGD(LOGTAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

	// Socket
	int socket = socket(res->ai_family, res->ai_socktype, 0);
	if (socket < 0) {
		ESP_LOGE(LOGTAG, "... Failed to allocate socket.");
		freeaddrinfo(res);
		return 1004;
	}
	ESP_LOGD(LOGTAG, "... allocated socket\r\n");

	// CONNECT
	if (connect(socket, res->ai_addr, res->ai_addrlen) != 0) {
		ESP_LOGE(LOGTAG, "... socket connect failed errno=%d", errno);
		close(socket);
		freeaddrinfo(res);
		return 1005;
	}
	ESP_LOGD(LOGTAG, "... connected");
	freeaddrinfo(res);

	// Build HTTP Request
	String sRequest;
	PrepareRequest(sRequest);


	// send HTTP request
	ESP_LOGD(LOGTAG, "sRequest: %s", sRequest.c_str());
	if (write(socket, sRequest.c_str(), sRequest.length()) < 0) {
		ESP_LOGE(LOGTAG, "... socket send failed");
		close(socket);
		return 1006;
	}
	sRequest.clear(); // free memory


	if (mpPostData) {
		if (write(socket, mpPostData, muPostDataSize) < 0) {
			ESP_LOGE(LOGTAG, "... socket send post data failed");
			close(socket);
			return 1007;
		}
	}


	ESP_LOGD(LOGTAG, "... socket send success");

	// Read HTTP response
	mHttpResponseParser.Init(mpDownloadHandler, muMaxResponseDataSize);

	//char recv_buf[1024];
	String sReceiveBuf;
	sReceiveBuf.resize(RECEIVE_BUFFER_SIZE);
	while (!mHttpResponseParser.ResponseFinished()) {
		size_t sizeRead = read(socket, (char*)sReceiveBuf.c_str(), sReceiveBuf.length());
		if (!mHttpResponseParser.ParseResponse((char*)sReceiveBuf.c_str(), sizeRead)) {
			ESP_LOGE(LOGTAG, "HTTP Response error: %d", mHttpResponseParser.GetError());
			close(socket);
			return 1008;
		}
	}

	//ESP_LOGI(LOGTAG, "data %i bytes: %s", mHttpResponseParser.GetContentLength(), mHttpResponseParser.GetBody().c_str());

	close(socket);

	return mHttpResponseParser.GetStatusCode();
}

unsigned short WebClient::HttpExecuteSecure() {

	String sRequest;

	char buf[512]; //TODO REMOVE EXTRA LARGE BUFFER AFTER TESTING
	String sReceiveBuf;

	int ret, flags, len;

	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_ssl_context ssl;
	mbedtls_x509_crt cacert;
	mbedtls_ssl_config conf;
	mbedtls_net_context server_fd;
	bool netInitDone = false;

	mbedtls_ssl_init(&ssl);
	mbedtls_x509_crt_init(&cacert);
	mbedtls_ctr_drbg_init(&ctr_drbg);

	ESP_LOGD(LOGTAG, "Seeding the random number generator");
	mbedtls_ssl_config_init(&conf);

	mbedtls_entropy_init(&entropy);

	if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0)) != 0) {
		ESP_LOGE(LOGTAG, "mbedtls_ctr_drbg_seed returned %d", ret);
		goto exit;
	}

	/*   ESP_LOGI(LOGTAG, "Loading the CA root certificate...");

	 ret = mbedtls_x509_crt_parse(&cacert, server_root_cert_pem_start,
	 server_root_cert_pem_end-server_root_cert_pem_start);

	 if(ret < 0)
	 {
	 ESP_LOGE(LOGTAG, "mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
	 abort();
	 } */

	ESP_LOGD(LOGTAG, "Setting hostname for TLS session...");

	/* Hostname set here should match CN in server certificate */
	if ((ret = mbedtls_ssl_set_hostname(&ssl, mpUrl->GetPortAsString().c_str())) != 0) {
		ESP_LOGE(LOGTAG, "mbedtls_ssl_set_hostname returned -0x%x", -ret);
		goto exit;
	}

	ESP_LOGD(LOGTAG, "Setting up the SSL/TLS structure...");

	if ((ret = mbedtls_ssl_config_defaults(&conf,
	MBEDTLS_SSL_IS_CLIENT,
	MBEDTLS_SSL_TRANSPORT_STREAM,
	MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
		ESP_LOGE(LOGTAG, "mbedtls_ssl_config_defaults returned %d", ret);
		goto exit;
	}

	/* MBEDTLS_SSL_VERIFY_OPTIONAL is bad for security, in this example it will print
	 a warning if CA verification fails but it will continue to connect.

	 You should consider using MBEDTLS_SSL_VERIFY_REQUIRED in your own code.
	 */
	mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
	mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
	mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
#ifdef CONFIG_MBEDTLS_DEBUG
	//mbedtls_esp_enable_debug_log(&conf, 4);
#endif

	if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) {
		ESP_LOGE(LOGTAG, "mbedtls_ssl_setup returned -0x%x\n\n", -ret);
		goto exit;
	}

	// the following onwards needs working WIFI and IP address
	mbedtls_net_init(&server_fd);
	netInitDone = true;

	ESP_LOGD(LOGTAG, "Connecting to %s:%hu...", mpUrl->GetHost().c_str(), mpUrl->GetPort());
	ESP_LOGD(LOGTAG, "Port as string '%s'", mpUrl->GetPortAsString().c_str());
	if ((ret = mbedtls_net_connect(&server_fd, mpUrl->GetHost().c_str(), mpUrl->GetPortAsString().c_str(), MBEDTLS_NET_PROTO_TCP))
			!= 0) {
		ESP_LOGE(LOGTAG, "mbedtls_net_connect returned -%x", -ret);
		goto exit;
	}

	ESP_LOGD(LOGTAG, "Connected.");

	mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

	ESP_LOGD(LOGTAG, "Performing the SSL/TLS handshake...");

	while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			ESP_LOGE(LOGTAG, "mbedtls_ssl_handshake returned -0x%x", -ret);
			goto exit;
		}
	}

	ESP_LOGD(LOGTAG, "Verifying peer X.509 certificate...");

	if ((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0) {
		/* In real life, we probably want to close connection if ret != 0 */
		ESP_LOGW(LOGTAG, "Failed to verify peer certificate!");
		bzero(buf, sizeof(buf));
		mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", flags);
		ESP_LOGW(LOGTAG, "verification info: %s", buf);
	} else {
		ESP_LOGD(LOGTAG, "Certificate verified.");
	}

	// Build HTTP Request

	PrepareRequest(sRequest);

	ESP_LOGD(LOGTAG, "Writing HTTP request... <%s>", sRequest.c_str());

	while ((ret = mbedtls_ssl_write(&ssl, (const unsigned char*)sRequest.c_str(), sRequest.length())) <= 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			ESP_LOGE(LOGTAG, "mbedtls_ssl_write returned -0x%x", -ret);
			goto exit;
		}
	}

	if (mpPostData) {
		while ((ret = mbedtls_ssl_write(&ssl, (const unsigned char*)mpPostData, muPostDataSize)) <= 0) {
			if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
				ESP_LOGE(LOGTAG, "mbedtls_ssl_write returned -0x%x during POST", -ret);
				goto exit;
			}
		}
	}


	//ESP_LOGI(LOGTAG, "%d bytes written", ret);
	ESP_LOGD(LOGTAG, "Reading HTTP response...");

	sRequest.clear(); // free memory

	//ESP_LOGI(LOGTAG, "... socket send success");

	// Read HTTP response
	mHttpResponseParser.Init(mpDownloadHandler, muMaxResponseDataSize);
	if (!sReceiveBuf.resize(RECEIVE_BUFFER_SIZE))
		ESP_LOGE(LOGTAG, "memory allocation failed (%d)", RECEIVE_BUFFER_SIZE);

	//ESP_LOGI(LOGTAG, "sReceiveBuf.length(%d)", sReceiveBuf.length());


	while (!mHttpResponseParser.ResponseFinished()) {
		//ESP_LOGI(LOGTAG, "before ssl_read");
		//ESP_LOGI(LOGTAG, "sReceiveBuf.length(%d), pointer=%p", sReceiveBuf.length(), sReceiveBuf.c_str());
		ret = mbedtls_ssl_read(&ssl, (unsigned char*)sReceiveBuf.c_str(), sReceiveBuf.length());
		//ESP_LOGI(LOGTAG, "after ssl_read ret=%d", ret);


		if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
			continue;

		if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
			ret = 0;
			break;
		}

		if (ret < 0) {
			//ESP_LOGE(LOGTAG, "Connection closed during parsing");
			ESP_LOGE(LOGTAG, "mbedtls_ssl_read returned -0x%x", -ret);
			break;
		}

		len = ret;

		//ESP_LOGI(LOGTAG, "invoking responseparse(buflen=%d)", len);
		if (!mHttpResponseParser.ParseResponse((char*)sReceiveBuf.c_str(), len)) {
			ESP_LOGE(LOGTAG, "HTTP Error Code: %d", mHttpResponseParser.GetError());
			goto exit;
		}
		//ESP_LOGI(LOGTAG, "responseparser actual length %d", mHttpResponseParser.GetContentLength());
	}

	mbedtls_ssl_close_notify(&ssl);

exit: 
	mbedtls_ssl_session_reset(&ssl);
	if (netInitDone) 
		mbedtls_net_free(&server_fd);

	mbedtls_x509_crt_free(&cacert);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

	if (!mHttpResponseParser.ResponseFinished() && ret != 0) {
		mbedtls_strerror(ret, buf, 100);
		ESP_LOGE(LOGTAG, "Last MBEDTLS error was: -0x%x - %s", -ret, buf);
		return 1010;
	}
	return mHttpResponseParser.GetStatusCode();

}
