#include <string.h>
#include <esp_system.h>
#include <esp_log.h>
#include <ping/ping_sock.h>
#include "Config.h"
#include "PingWatchdog.h"

const char tag[] = "ping";

void fPingWatchdogTask(void *pvParameter)
{
  ((PingWatchdog *)pvParameter)->PingWatchdogTask();
  vTaskDelete(NULL);
}

void onPingSuccess(esp_ping_handle_t hdl, void *args)
{
  ((PingWatchdog *)args)->Success();
};

void onPingTimeout(esp_ping_handle_t hdl, void *args)
{
  ((PingWatchdog *)args)->Timeout();
};

void onPingEnd(esp_ping_handle_t hdl, void *args)
{
  ((PingWatchdog *)args)->End();
};

void PingWatchdog::Start(ip4_addr_t ipAddress)
{

  /// DEFAULTS ***
  //    .count = 5,
  //    .interval_ms = 1000,
  //    .timeout_ms = 1000,
  //    .data_size = 64,
  //    .tos = 0,
  //    .target_addr = *(IP_ANY_TYPE),
  //    .task_stack_size = 2048,
  //    .task_prio = 2,
  //    .interface = 0,

  mPingConfig = ESP_PING_DEFAULT_CONFIG();
  mPingConfig.target_addr.u_addr.ip4 = ipAddress;
  mPingConfig.target_addr.type = IPADDR_TYPE_V4; // target IP address
  mPingConfig.count = 100;

  mPingCallbacks.on_ping_success = onPingSuccess;
  mPingCallbacks.on_ping_timeout = onPingTimeout;
  mPingCallbacks.on_ping_end = onPingEnd;
  mPingCallbacks.cb_args = this; // arguments that will feed to all callback functions, can be NULL

  xTaskCreate(&fPingWatchdogTask, "PingWatchdog", 1024 * 2, this, ESP_TASK_TIMER_PRIO, NULL); // large stack is needed
}


PingWatchdog::PingWatchdog()
{
}

void PingWatchdog::Success()
{
  ESP_LOGI(tag, "ping success");
  muSuccess++;
  esp_ping_stop(mhPingSession);
}

void PingWatchdog::Timeout()
{
  ESP_LOGI(tag, "ping timeout");
}

void PingWatchdog::End()
{
  ESP_LOGI(tag, "ping end");
}

void PingWatchdog::PingWatchdogTask()
{
  ESP_LOGI(tag, "Watchdog Task Started");
	vTaskDelay(1000 / portTICK_PERIOD_MS);
  ESP_LOGI(tag, "before esp ping new session");

  esp_err_t err = ESP_OK;
  if (ESP_OK != (err = esp_ping_new_session(&mPingConfig, &mPingCallbacks, &mhPingSession))) {
    ESP_LOGE(tag, "error creating  ping session: %d, %s", err, esp_err_to_name(err));
  }
  ESP_LOGI(tag, "after esp ping new session");
  
  while (true)
  {
    esp_ping_start(mhPingSession);
		vTaskDelay(10 * 1000 / portTICK_PERIOD_MS);
    esp_ping_stop(mhPingSession);
    if (muSuccess) {
      muSuccess = 0;
      ESP_LOGI(tag, "Resetting watchdog.");
    } else {
      ESP_LOGW(tag, "Could not ping Gateway. Rebooting.");
      esp_restart();
    }
  }
}

PingWatchdog::~PingWatchdog()
{
}
