#include "esp_log_level.h"
#include <alloca.h>
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

Encode_Buffer encoded_buffer = (Encode_Buffer){.buffer_len = 255};

char topic[100];

// Tag for print debugging;
#define PB_ENCODE "PB_ENCODE"


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

bool Create_N_Encode_Device(){
    Sparkplug_Node *node = (Sparkplug_Node *)alloca(sizeof(Sparkplug_Node) + sizeof(Sparkplug_Device));
    *node = (Sparkplug_Node){
        .nodeID = "ESP32C6_EoN",
        .groupID = "mylab",
        .namespace = "spBv1.0",
        .bdseq = 0,
        .seq = 0,
        .NCMD_bit_mark = (EN_NCMD_REBIRTH | EN_NCMD_REBOOT | EN_NCMD_SCAN_RATE),
    };


    Sparkplug_Device device = (Sparkplug_Device){
        .deviceID = "TheDevice",
        .DCMD_bit_mark = (EN_DCMD_REBIRTH | EN_DCMD_REBOOT),
    };


    node->devices[0] = device;

    device.DBIRTH = Create_Metrics(2);
    if (!device.DBIRTH)
        return false;

    time_t now;
    time(&now);

    // Device topic namespace
    Device_Generate_All_Topic_Namespace(node, &device);
    Device_Fill_DCMDs_DBIRTH_Metrics(&device, &now);

    // copy topic outside for later use
    strncpy(topic, device.Topic_DBIRTH, strlen(device.Topic_DBIRTH));

    // add DDATA
    device.DDATA = Create_Metrics(2);
    if (!device.DDATA) {
        return false;
    }


    sparkplug_payload_metric* metric = add_metric(device.DDATA);
    *metric = (sparkplug_payload_metric){
        .has_timestamp = 1,
        .has_datatype = 1,
        .is_null = 0,

        .name.arg = "Input/1",
        .name.funcs.encode = &encode_string,

        .datatype = org_eclipse_tahu_protobuf_DataType_Boolean,
        .value.boolean_value = 0,
        .which_value = org_eclipse_tahu_protobuf_Payload_Metric_boolean_value_tag,
        .timestamp = now,
    };

    metric = add_metric(device.DDATA);
    *metric = (sparkplug_payload_metric){
        .has_timestamp = 1,
        .has_datatype = 1,
        .is_null = 0,

        .name.arg = "Input/2",
        .name.funcs.encode = &encode_string,

        .datatype = org_eclipse_tahu_protobuf_DataType_Boolean,
        .value.boolean_value = 0,
        .which_value = org_eclipse_tahu_protobuf_Payload_Metric_boolean_value_tag,
        .timestamp = now,
    };

    Parse_DDATA_Into_DBIRTH(&device);

    sparkplug_payload payload = {
        .metrics.arg = device.DBIRTH,
        .metrics.funcs.encode = &encode_metrics,
        .has_timestamp = 1,
        .timestamp = now,
        .seq = 1,
        .has_seq = 1,
    };

    printf("encode payload: %d\n", encode_payload(&encoded_buffer, &payload));
    
    // pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    // if(!pb_encode(&stream, org_eclipse_tahu_protobuf_Payload_fields, &payload))
    //     return false;

    // message_length = stream.bytes_written;
    // ESP_LOGE(PB_ENCODE, "Encoded  %zu -> %zu bytes", sizeof(*device.DBIRTH->metrics), stream.bytes_written);
    ESP_LOGE(PB_ENCODE, "Encoded  %zu -> %zu bytes", sizeof(*device.DBIRTH->metrics), encoded_buffer.encoded_length);

    free_device(&device);
    return true;
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            sntp_service_init(&ntp);
            ESP_LOGI("ENCODE", "%d", Create_N_Encode_Device());
            esp_mqtt_client_publish(client, topic, (const char*)encoded_buffer.buffer, encoded_buffer.encoded_length, 1, 0);
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI("SUB", "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI("PUB", "MQTT_EVENT_PUBLISHED, data_len=%d", event->total_data_len);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI("DATA", "MQTT_EVENT_DATA");
            break;

        default:
            break;
    }
}

void app_main() {
    nvs_flash_init();
    wifi_init("cornhub", "headbanger");

    for(;;){
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
