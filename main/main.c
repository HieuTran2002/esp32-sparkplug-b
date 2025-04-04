#include "esp_log_level.h"
#include "pb.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include <stdint.h>
#include <stdio.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "smallplug_b.pb.h"

bool encode_name(pb_ostream_t *stream, const pb_field_t *field, void * const *arg){
    ESP_LOGI("ENCODE", "name");
    char* string = (char*)*arg;

    if (!pb_encode_string(stream, (uint8_t *)string, strlen(string))) {
        return false;
    }
    ESP_LOGI("ENCODE", "name done");
    return true;
}

bool encode_metrics(pb_ostream_t *stream, const pb_field_t *field, void * const *arg){
    ESP_LOGI("ENCODE", "metrics");
    smallplug_Payload_Metric *metric = (smallplug_Payload_Metric*)*arg;
    metric->datatype = smallplug_DataType_Int16;
    metric->value.int_value = 123456;
    metric->name.arg = "hello world";
    metric->name.funcs.encode = &encode_name;

    if(!pb_encode(stream, smallplug_Payload_Metric_fields, metric)){
        return false;
    }
    ESP_LOGI("ENCODE", "metrics done");
    return true;
}

int message_init(void){
    uint8_t buffer[255];  // Output buffer
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

    smallplug_Payload payload = smallplug_Payload_init_default;
    smallplug_Payload_Metric metric = smallplug_Payload_Metric_init_default;

    payload.seq = 0;
    payload.has_seq = 1;
    payload.has_timestamp = 1;
    payload.has_timestamp = 123456789;
    payload.metrics.arg = &metric;
    payload.metrics.funcs.encode = &encode_metrics;

    ESP_LOGI("SETUP", "Initial");
    if(!pb_encode(&stream, smallplug_Payload_fields, &payload)){
        return false;
    }
    ESP_LOGI("SETUP", "ALL DONE");
    printf("Encoded message size: %zu bytes\n", stream.bytes_written);
    return true;
}

void app_main() {
    nvs_flash_init();
    ESP_LOGI("ENCODE", "%d", message_init());
}

