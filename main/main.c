#include "esp_log_level.h"
#include <stdlib.h>
#include <reent.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "freertos/idf_additions.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "portmacro.h"
#include "sparkplug_b.pb.h"
#include "wifi_helper.h"
#include "mqtt_helper.h"
#include "simple_SNTP.h"

#include "pb.h"
#include "pb_encode.h"
#include "sparkplug_b.h"

ntp_time ntp = {};

uint8_t buffer[255];  // Output buf
size_t message_length;

char topic[100];

// Tag for print debugging;
#define PB_ENCODE "PB_ENCODE"

int nbirth_example(void){
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

    time_t now;
    time(&now);

    Metrics *nbirth = Create_Metrics(2);
    if(!nbirth) return false;

    // Node Control/Rebirth
    sparkplug_payload_metric *rebirth = add_metric(nbirth);
    Place_NCMD_Metric(rebirth, &now, NCMD_REBIRTH);

    // Node Control/Reboot
    sparkplug_payload_metric *reboot = add_metric(nbirth);
    Place_NCMD_Metric(reboot, &now, NCMD_REBOOT);

    // bdseq
    sparkplug_payload_metric *bdseq = add_metric(nbirth);
    Place_bdsep_Metric(bdseq, &now);

    // Node Control/Scan Rate
    sparkplug_payload_metric *scan_rate = add_metric(nbirth);
    Place_NCMD_Metric(scan_rate, &now, NCMD_SCAN_RATE);

    // Properties/Hardware Model
    sparkplug_payload_metric *hw_model = add_metric(nbirth);
    Place_Properties_HW_Model_Metric(hw_model, &now, "ESP32-C6");

    ESP_LOGI("ENCODE", "START %d", nbirth->used);

    org_eclipse_tahu_protobuf_Payload payload = {
        .metrics.arg = nbirth,
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
    ESP_LOGE(PB_ENCODE, "Encoded  %zu -> %zu bytes", sizeof(*nbirth->metrics), stream.bytes_written);

    free_metrics(nbirth);
    return true;
}

bool Create_N_Encode_Node(){
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    Sparkplug_Node node = {
        .nodeID = "ESP32C6_EoN",
        .groupID = "mylab",
        .namespace = "spBv1.0",
        .bdseq = 0,
        .seq = 0,
        .NCMD_bit_mark = (EN_NCMD_REBIRTH | EN_NCMD_REBOOT | EN_NCMD_SCAN_RATE),
    };

    sparkplug_payload* payload = Node_Auto_Generate_NBIRTH_payload(&node);
    Node_Generate_All_Topic_Namespace(&node);

    strncpy(topic, node.Topic_NBIRTH, strlen(node.Topic_NBIRTH));

    if(!pb_encode(&stream, org_eclipse_tahu_protobuf_Payload_fields, payload))
        return false;

    message_length = stream.bytes_written;
    ESP_LOGE(PB_ENCODE, "Encoded  %zu -> %zu bytes", sizeof(*node.NBIRTH->metrics), stream.bytes_written);

    // Free payload after successfully encode.
    free(payload);
    free_metrics(node.NBIRTH);
    return 1;
}

bool Generate_N_Encode_Device(){
    Sparkplug_Node node = {
        .nodeID = "ESP32C6_EoN",
        .groupID = "mylab",
        .namespace = "spBv1.0",
        .bdseq = 0,
        .seq = 0,
        .NCMD_bit_mark = (EN_NCMD_REBIRTH | EN_NCMD_REBOOT | EN_NCMD_SCAN_RATE),
    };

    Sparkplug_Device device = {
        .deviceID = "TheDevice",
    };
    node.devices[0] = device;

    // Device topic namespace
    Node_Generate_All_Topic_Namespace(&node);
    Device_Generate_All_Topic_Namespace(&node, &device);
    strncpy(topic, device.Topic_DBIRTH, strlen(device.Topic_DBIRTH));

    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    device.DBIRTH = Create_Metrics(2);
    if (!device.DBIRTH)
        return false;

    sparkplug_payload_metric* metric = add_metric(device.DBIRTH);

    time_t now;
    time(&now);

    *metric = (sparkplug_payload_metric){
        .has_timestamp = 1,
        .has_datatype = 1,
        .is_null = 0,

        .name.arg = "the name",
        .name.funcs.encode = &encode_string,

        .datatype = org_eclipse_tahu_protobuf_DataType_Boolean,
        .value.boolean_value = 1,
        .which_value = org_eclipse_tahu_protobuf_Payload_Metric_boolean_value_tag,
        .timestamp = now,
    };


    org_eclipse_tahu_protobuf_Payload payload = {
        .metrics.arg = device.DBIRTH,
        .metrics.funcs.encode = &encode_metrics,
        .has_timestamp = 1,
        .timestamp = now,
        .seq = 1,
        .has_seq = 1,
    };


    if(!pb_encode(&stream, org_eclipse_tahu_protobuf_Payload_fields, &payload))
        return false;

    message_length = stream.bytes_written;
    ESP_LOGE(PB_ENCODE, "Encoded  %zu -> %zu bytes", sizeof(*device.DBIRTH), stream.bytes_written);

    free_metrics(device.DBIRTH);
    return true;
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            sntp_service_init(&ntp);
            ESP_LOGI("ENCODE", "%d", Generate_N_Encode_Device());
            esp_mqtt_client_publish(client, topic, (const char*)buffer, message_length, 2, 0);
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI("SUB", "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI("PUB", "MQTT_EVENT_PUBLISHED, data_len=%d", event->data_len);
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
