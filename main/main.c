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
#include <stdlib.h>
#include <sys/time.h>

ntp_time ntp = {};

uint8_t buffer[255];  // Output buf
size_t message_length;

// Tag for print debugging;
#define PB_ENCODE "PB_ENCODE"

Metrics* Create_Metrics(uint8_t _capacity){
    Metrics *m = (Metrics*)malloc(sizeof(Metrics));
    if(!m) return NULL;
    m->capacity = _capacity;
    m->metrics = (sparkplug_payload_metric*)malloc(sizeof(sparkplug_payload_metric) * _capacity);
    if(!m->metrics) return NULL;
    return m;
}

int message_init(void){
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

    time_t now;
    time(&now);

    Metrics *metrics = Create_Metrics(2);
    metrics->used = 0;
    if(!metrics) return false;

    sparkplug_payload_metric *birth = add_metric(metrics);

    *birth = (sparkplug_payload_metric){
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

    sparkplug_payload_metric *bdseq = add_metric(metrics);
    *bdseq = (sparkplug_payload_metric){
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

    sparkplug_payload_metric *hw_model = add_metric(metrics);
    *hw_model = (sparkplug_payload_metric){
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

    ESP_LOGI("ENCODE", "START %d", metrics->used);

    org_eclipse_tahu_protobuf_Payload payload = {
        .metrics.arg = metrics,
        .metrics.funcs.encode = &encode_metrics,
        .has_timestamp = 1,
        .timestamp = now,
        .seq = 0,
        .has_seq = 1,
    };

    if(!pb_encode(&stream, org_eclipse_tahu_protobuf_Payload_fields, &payload)){
        return false;
    }

    message_length = stream.bytes_written;
    ESP_LOGE(PB_ENCODE, "Encoded  %zu -> %zu bytes", sizeof(*metrics->metrics), stream.bytes_written);

    free_metrics(metrics);
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
            esp_mqtt_client_publish(client, "spBv1.0/NBIRTH", (const char*)buffer, message_length, 2, 0);
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

    for(;;){
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
