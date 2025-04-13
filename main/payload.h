#ifndef PAYLOAD_H
#define PAYLOAD_H

#include "sparkplug_b.pb.h"
#include <stdint.h>

#define METRICS_GROWTH_RATE 4
#define NAMESPACE "spBv1.0"

#define sparkplug_payload_metric org_eclipse_tahu_protobuf_Payload_Metric
#define sparkplug_payload org_eclipse_tahu_protobuf_Payload

// Default info about device
#define HW_MODEL "ESP32-C6"

typedef struct{
    uint8_t used;
    uint8_t capacity;
    uint8_t auto_expand;
    sparkplug_payload_metric *metrics;
} Metrics;

typedef struct {
    const char* deviceID;
    Metrics *metrics;
} sp_device;


bool encode_string(pb_ostream_t *stream, const pb_field_t *field, void * const *arg);
bool encode_metrics(pb_ostream_t *stream, const pb_field_t *field, void * const *arg);
sparkplug_payload_metric *add_metric(Metrics *m);
void free_metrics(Metrics* m);
Metrics* Create_Metrics(uint8_t _capacity);
#endif /* end of include guard: PAYLOAD_H */
