#include "esp_log_level.h"
#include "freertos/idf_additions.h"
#include "mqtt_client.h"
#include "node.h"
#include "pb.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include <reent.h>
#include <stdint.h>
#include <stdio.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "portmacro.h"
#include "sparkplug_b.pb.h"
#include "wifi_helper.h"
#include "mqtt_helper.h"
#include "simple_SNTP.h"
#include <sys/time.h>

ntp_time ntp = {};


uint8_t buffer[255];  // Output buf
size_t message_length;

int message_init(void){
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

    time_t now;
    time(&now);

    org_eclipse_tahu_protobuf_Payload_Metric rebirth = {
        .is_null = false,
        .has_timestamp = true,
        .timestamp = now,
        .datatype = org_eclipse_tahu_protobuf_DataType_Boolean,
        .value.boolean_value = 0,
        .name.arg = "Node Control/Rebirth",
        .name.funcs.encode = &encode_string,
        .which_value = org_eclipse_tahu_protobuf_Payload_Metric_boolean_value_tag,
        .has_datatype = true,
    };

    org_eclipse_tahu_protobuf_Payload_Metric bdSeq = {
        .is_null = false,
        .has_timestamp = true,
        .timestamp = now,
        .datatype = org_eclipse_tahu_protobuf_DataType_Int32,
        .value.int_value = 0,
        .name.arg = "bdSeq",
        .name.funcs.encode = &encode_string,
        .which_value = org_eclipse_tahu_protobuf_Payload_Metric_int_value_tag,
        .has_datatype = true,
    };

    org_eclipse_tahu_protobuf_Payload_Metric hw_model = {
        .is_null = false,
        .has_timestamp = true,
        .timestamp = now,
        .datatype = org_eclipse_tahu_protobuf_DataType_String,
        .value.string_value.arg = "ESP32C6",
        .value.string_value.funcs.encode = &encode_string,
        .name.arg = "Properties/Hardware Model",
        .name.funcs.encode = &encode_string,
        .which_value = org_eclipse_tahu_protobuf_Payload_Metric_string_value_tag,
        .has_datatype = true,
    };

    Metrics metrics = {
        .size = 3,
    };
    metrics.metrics[0] = rebirth;
    metrics.metrics[1] = bdSeq;
    metrics.metrics[2] = hw_model;

    org_eclipse_tahu_protobuf_Payload payload = {
        .metrics.arg = &metrics,
        //.metrics.arg = &metric,
        .metrics.funcs.encode = &encode_metrics,
        .has_timestamp = 1,
        .timestamp = now,
        .seq = 0,
        .has_seq = 1,
    };

    ESP_LOGI("SETUP", "Initial");
    if(!pb_encode(&stream, org_eclipse_tahu_protobuf_Payload_fields, &payload)){
        return false;
    }

    message_length = stream.bytes_written;
    ESP_LOGI("SETUP", "ALL DONE");
    printf("Encoded message size: %zu bytes\n", stream.bytes_written);
    return true;
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            sntp_service_init(&ntp);
            ESP_LOGI("ENCODE", "%d", message_init());
            esp_mqtt_client_publish(client, "spBv1.0/NBIRTH", (const char*)&buffer, message_length, 2, 0);
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI("SUB", "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI("PUB", "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI("DATA", "MQTT_EVENT_DATA");
            break;
    }
}

void app_main() {
    nvs_flash_init();
    wifi_init("HCTL", "123456789HCTL");

    ESP_LOGE("ENCODE", "%s", (char*)buffer);

    for(;;){
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
