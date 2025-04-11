#ifndef MQTT_HELPER_H
#define MQTT_HELPER_H


#include "esp_event_base.h"
#include "esp_netif_ip_addr.h"
#include <esp_log.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <mqtt_client.h>
#include <stdint.h>

typedef struct{
    esp_ip4_addr_t IP;
    uint16_t Port;

} MQTT_Broker_Address_t;

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
void mqtt_init();
#endif /* end of include guard: MQTT_HELPER_H */
