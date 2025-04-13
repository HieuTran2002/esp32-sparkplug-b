#include "wifi_helper.h"
#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_log_level.h"
#include "esp_wifi_types_generic.h"
#include "mqtt_helper.h"
#include <stdint.h>
#include <string.h>

const char* WIFI = "WIFI";


void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    if (event_base == WIFI_EVENT){
        if(event_id == WIFI_EVENT_STA_CONNECTED){
            ESP_LOGI(WIFI, "Connnected to AP");
        }
    }

    else if(event_base == IP_EVENT){
        if(event_id == IP_EVENT_STA_GOT_IP){
            mqtt_init();
        }
    }
}


void wifi_init(char *ssid, char *pwd) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    // esp_event_loop_handle_t event_loop;
    // esp_event_handler_register_with(&event_loop, ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, run_on_event, NULL);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // Setup event handler for wifi
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                ESP_EVENT_ANY_ID,
                &wifi_event_handler,
                NULL,
                &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                IP_EVENT_STA_GOT_IP,
                &wifi_event_handler,
                NULL,
                &instance_got_ip));


    // Config STA` wifi`
    esp_wifi_set_mode(WIFI_MODE_STA);
    wifi_config_t wifi_config = { };

    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0'; // Ensure null-termination

    strncpy((char *)wifi_config.sta.password, pwd, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0'; // Ensure null-termination
                                                                       
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    esp_wifi_connect();
}

