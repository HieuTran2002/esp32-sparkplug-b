#include "mqtt_helper.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* topic = NULL;
void trigger_extra_on_data_event(void* event_data);

void mqtt_init(){
    char result[50];

    // Convert esp_ip4_addr_t to char*

    sprintf(result,"mqtt://%s:%d", "192.168.0.109", 1883);
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = result,
        .credentials.client_id = "theid",
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

