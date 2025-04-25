#include <alloca.h>
#include <stdlib.h>
#include <reent.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "esp_log_level.h"
#include "freertos/idf_additions.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "portmacro.h"
#include "sparkplug_b.pb.h"
#include "wifi_helper.h"
#include "mqtt_helper.h"
#include "simple_SNTP.h"
#include "esp_log_expansion.h"

#include "pb.h"
#include "pb_encode.h"
#include "sparkplug_b.h"

ntp_time ntp = {};
uint8_t buffer[2];  // Output buf
size_t message_length;

Encode_Buffer nbirth = (Encode_Buffer){.buffer_len = 255};
Encode_Buffer dbirth = (Encode_Buffer){.buffer_len = 255};
Encode_Buffer ndata  = (Encode_Buffer){.buffer_len = 255};
Encode_Buffer ddata  = (Encode_Buffer){.buffer_len = 255};

Sparkplug_Node *node;
Sparkplug_Device *device;

// Tag for print debugging;
#define PB_ENCODE "PB_ENCODE"
#define DEBUG "DEBUG"


bool Create_N_Encode_Node(){
    node = (Sparkplug_Node*)malloc(sizeof(Sparkplug_Node));
    *node = (Sparkplug_Node){
        .nodeID = "ESP32C6 EoN",
        .groupID = "Home lab",
        .namespace = "spBv1.0",
        .bdseq = 0,
        .seq = 0,
        .NCMD_bit_mark = (EN_NCMD_REBIRTH | EN_NCMD_REBOOT | EN_NCMD_SCAN_RATE),
    };

    time_t now;
    time(&now);

    Node_Generate_All_Topic_Namespace(node);

    // add NCMD
    node->NCMD = Create_Metrics(4);
    Node_Fill_NCMDs_Metrics(node, &now);

    // add NDATA
    node->NDATA = Create_Metrics(1);
    *(add_metric(node->NDATA)) = (sparkplug_payload_metric){
        .has_timestamp = 1,
        .has_datatype = 1,
        .is_null = 0,

        .name.arg = "CPU Temperature",
        .name.funcs.encode = &encode_string,

        .datatype = org_eclipse_tahu_protobuf_DataType_Int16,
        .value.int_value = 0,
        .which_value = org_eclipse_tahu_protobuf_Payload_Metric_int_value_tag,
        .timestamp = now,
    };

    Metrics *stack_metrics[2] = {node->NCMD, node->NDATA};
    Stack_Metrics_ptrs stack_metrics_ptrs =  (Stack_Metrics_ptrs){
        .Mul_Metrics_ptrs = stack_metrics,
        .Count = 2,
    };

    sparkplug_payload payload = {
        .metrics.arg = &stack_metrics_ptrs,
        .metrics.funcs.encode = &encode_multiple_metrics_ptr,
        .has_timestamp = 1,
        .timestamp = now,
        .seq = 0,
        .has_seq = 1,
    };

    printf("encode payload: %d\n", encode_payload(&nbirth, &payload));
    return 1;
}


bool Create_N_Encode_Device(){
    // create device
    device = (Sparkplug_Device*)malloc(sizeof(Sparkplug_Device));

    // add device manager to node
    node->Device_Manager = create_device_manager(2);
    add_device(node->Device_Manager, device);

    *device = (Sparkplug_Device){
        .deviceID = "ESP32 C6",
        .DCMD_bit_mark = (EN_DCMD_REBIRTH | EN_DCMD_REBOOT),
    };

    ESP_LOGI_BrightYellow(DEBUG, "Device in node %s\n", node->Device_Manager->devices[0]->deviceID);

    time_t now;
    time(&now);

    device->DCMD = Create_Metrics(1);
    if (!device->DCMD) return false;

    // Device topic namespace
    Device_Generate_All_Topic_Namespace(node, device);
    Device_Fill_DCMDs_Metrics(device, &now);

    // add DDATA
    device->DDATA = Create_Metrics(1);
    if (!device->DDATA) return false;

    *add_metric(device->DCMD) = (sparkplug_payload_metric){
        .has_timestamp = 1,
        .has_datatype = 1,
        .is_null = 0,

        .name.arg = "Output/LED 1",
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

    sparkplug_payload payload = {
        .metrics.arg = &stack_metrics,
        .metrics.funcs.encode = &encode_multiple_metrics_ptr,
        .has_timestamp = 1,
        .timestamp = now,
        .seq = 1,
        .has_seq = 1,
    };

    printf("encode payload: %d\n", encode_payload(&dbirth, &payload));

    ESP_LOGE(PB_ENCODE, "Encoded  %zu -> %zu bytes", sizeof(*device->DDATA->metrics), dbirth.encoded_length);

    return true;
}

void Create_N_Encode_NDATA(){
    if(!node) return;
    if(!node->NDATA) return;

    ESP_LOGI_BrightCyan(DEBUG, "NDATA available %s", (char *)node->NDATA->metrics->name.arg);
    ESP_LOGI_BrightCyan(DEBUG, "NDATA available %d", node->NDATA->used);

    time_t now;
    time(&now);

    node->NDATA->metrics->value.int_value = 70;
    node->NDATA->metrics->timestamp = now;

    sparkplug_payload payload = {
        .metrics.arg = &node->NDATA,
        .metrics.funcs.encode = &encode_metrics,
        .has_timestamp = 1,
        .timestamp = now,
        .seq = 2,
        .has_seq = 1,
    };


    printf("encode payload: %d\n", encode_payload(&ndata, &payload));

    ESP_LOGE(PB_ENCODE, "Encoded  %zu -> %zu bytes", sizeof(*node->NDATA->metrics), ndata.encoded_length);
}

void Create_N_Encode_DDATA(){
    if(!device) return;
    if(!device->DDATA) return;

    ESP_LOGI_BrightCyan(DEBUG, "DDATA available %s", (char *)device->DDATA->metrics->name.arg);
    ESP_LOGI_BrightCyan(DEBUG, "DDATA available %d", device->DDATA->used);

    time_t now;
    time(&now);

    device->DDATA->metrics->value.boolean_value = 1;
    device->DDATA->metrics->timestamp = now;

    sparkplug_payload payload = {
        .metrics.arg = &device->DDATA,
        .metrics.funcs.encode = &encode_metrics,
        .has_timestamp = 1,
        .timestamp = now,
        .seq = 2,
        .has_seq = 1,
    };


    printf("encode payload: %d\n", encode_payload(&ddata, &payload));

    ESP_LOGE(PB_ENCODE, "Encoded  %zu -> %zu bytes", sizeof(*device->DDATA->metrics), ddata.encoded_length);
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            esp_mqtt_client_publish(client, node->Topic_NBIRTH, (const char*)nbirth.buffer, nbirth.encoded_length, 1, 0);
            esp_mqtt_client_publish(client, device->Topic_DBIRTH, (const char*)dbirth.buffer, dbirth.encoded_length, 1, 0);
            esp_mqtt_client_publish(client, node->Topic_NDATA, (const char*)ndata.buffer, ndata.encoded_length, 1, 0);
            esp_mqtt_client_publish(client, device->Topic_DDATA, (const char*)ddata.buffer, ddata.encoded_length, 1, 0);
            free_node(node);

            break;

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

    ESP_LOGI("ENCODE", "%d", Create_N_Encode_Node());
    ESP_LOGI("ENCODE", "%d", Create_N_Encode_Device());

    Create_N_Encode_NDATA();
    Create_N_Encode_DDATA();

    mqtt_init();
}

void app_main() {
    nvs_flash_init();
    wifi_init("HCTL", "123456789HCTL");
    register_wifi_connected_callback(wifi_on_connected_handle);

    for(;;){
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
