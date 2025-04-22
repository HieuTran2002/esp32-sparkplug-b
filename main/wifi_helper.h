#ifndef WIFI_HELPER_H
#define WIFI_HELPER_H

#include "esp_event_base.h"
#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include <esp_http_server.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <esp_event.h>
#include <esp_netif.h>

void wifi_init(char *ssid, char *pwd);

typedef void(* wifi_event_callback_t)(void* event_data);
void register_wifi_connected_callback(wifi_event_callback_t callback);

// event callbackfor wifi event
// ** only has event_data since IP is what mostly needed

#endif /* end of include guard: WIFI_HELPER_H */
