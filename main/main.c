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

uint8_t buffer[2];  // Output buf
size_t message_length;

Encode_Buffer encoded_buffer = (Encode_Buffer){.buffer_len = 255};

char topic[100];

// Tag for print debugging;
#define PB_ENCODE "PB_ENCODE"


bool Create_N_Encode_Node(){
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    Sparkplug_Node node = {
        .nodeID = "ESP32C6 EoN",
        .groupID = "Home lab",
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
    Sparkplug_Node *node = (Sparkplug_Node *)malloc(sizeof(Sparkplug_Node) + sizeof(Sparkplug_Device));
    *node = (Sparkplug_Node){
        .nodeID = "ESP32C6 EoN",
        .groupID = "Home lab",
        .namespace = "spBv1.0",
        .bdseq = 0,
        .seq = 0,
        .NCMD_bit_mark = (EN_NCMD_REBIRTH | EN_NCMD_REBOOT | EN_NCMD_SCAN_RATE),
    };


    Sparkplug_Device *device = (Sparkplug_Device*)malloc(sizeof(Sparkplug_Device));

    *device = (Sparkplug_Device){
        .deviceID = "ESP32 C6",
        .DCMD_bit_mark = (EN_DCMD_REBIRTH | EN_DCMD_REBOOT),
    };


    device->DCMD = Create_Metrics(2);
    if (!device->DCMD) return false;

    time_t now;
    time(&now);

    // Device topic namespace
    Device_Generate_All_Topic_Namespace(node, device);
    Device_Fill_DCMDs_Metrics(device, &now);

    // copy topic outside for later use
    strncpy(topic, device->Topic_DBIRTH, strlen(device->Topic_DBIRTH));

    // add DDATA
    device->DDATA = Create_Metrics(2);
    if (!device->DDATA) return false;

    *add_metric(device->DDATA) = (sparkplug_payload_metric){
        .has_timestamp = 1,
        .has_datatype = 1,
        .is_null = 0,

        .name.arg = "Input/Button 1",
        .name.funcs.encode = &encode_string,

        .datatype = org_eclipse_tahu_protobuf_DataType_Boolean,
        .value.boolean_value = 0,
        .which_value = org_eclipse_tahu_protobuf_Payload_Metric_boolean_value_tag,
        .timestamp = now,
    };

    *add_metric(device->DDATA) = (sparkplug_payload_metric){
        .has_timestamp = 1,
        .has_datatype = 1,
        .is_null = 0,

        .name.arg = "Input/Button 2",
        .name.funcs.encode = &encode_string,

        .datatype = org_eclipse_tahu_protobuf_DataType_Boolean,
        .value.boolean_value = 0,
        .which_value = org_eclipse_tahu_protobuf_Payload_Metric_boolean_value_tag,
        .timestamp = now,
    };

    device->Properties = (Metrics*)Create_Metrics(1);
    if(!device->Properties) return false;

    *add_metric(device->Properties) = (sparkplug_payload_metric){
        .has_timestamp = 1,
        .has_datatype = 1,
        .is_null = 0,

        .name.arg = "Properties/Hardware Make",
        .name.funcs.encode = &encode_string,

        .datatype = org_eclipse_tahu_protobuf_DataType_String,
        .value.string_value.arg = "Espressif",
        .value.string_value.funcs.encode = &encode_string,
        .which_value = org_eclipse_tahu_protobuf_Payload_Metric_string_value_tag,
        .timestamp = now,
    };


    Metrics *array_metrics[3] = {device->DCMD, device->DDATA, device->Properties};
    Stack_Metrics_ptrs stack_metrics = (Stack_Metrics_ptrs){
        .Count = 3,
        .Mul_Metrics_ptrs = array_metrics,
    };
    // Merge_Metrics(array_metrics, 2);

    sparkplug_payload payload = {
        .metrics.arg = &stack_metrics,
        .metrics.funcs.encode = &encode_multiple_metrics_ptr,
        .has_timestamp = 1,
        .timestamp = now,
        .seq = 1,
        .has_seq = 1,
    };

    printf("encode payload: %d\n", encode_payload(&encoded_buffer, &payload));

    printf("DDATA %zu\n", sizeof(*device->DDATA->metrics));
    printf("DDATA %zu\n", sizeof(*device->DCMD->metrics));
    printf("DDATA %zu\n", sizeof(*device->Properties->metrics));
    
    ESP_LOGE(PB_ENCODE, "Encoded  %zu -> %zu bytes", sizeof(*device->DDATA->metrics), encoded_buffer.encoded_length);

    free_device(device);
    free(node);

    return true;
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            esp_mqtt_client_publish(client, topic, (const char*)encoded_buffer.buffer, encoded_buffer.encoded_length, 1, 0);

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI("PUB", "MQTT_EVENT_PUBLISHED, data_len=%d", event->data_len);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI("DATA", "MQTT_EVENT_DATA");
            break;

        default:
            break;
    }
}

void wifi_on_connected_handle(void* event_data){
    sntp_service_init(&ntp);
    mqtt_init();

    ESP_LOGI("ENCODE", "%d", Create_N_Encode_Device());
}

void app_main() {
    nvs_flash_init();
    wifi_init("HCTL", "123456789HCTL");
    register_wifi_connected_callback(wifi_on_connected_handle);

    for(;;){
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
