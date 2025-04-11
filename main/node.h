#ifndef NODE_H
#define NODE_H

#include "sparkplug_b.pb.h"

#define NAMESPACE "spBv1.0"

#define sparkplug_payload_metric org_eclipse_tahu_protobuf_Payload_Metric
#define sparkplug_payload org_eclipse_tahu_protobuf_Payload

typedef struct{
    size_t size;
    sparkplug_payload_metric *metrics;
} Metrics;

typedef struct {
    const char* deviceID;
    Metrics *metrics;
} sp_device;

typedef struct {
    const char* groupID;
    const char* nodeID;
    int seq;
    int bdseq;
    Metrics *metrics;
    sp_device devices[];
} sp_node;

bool encode_string(pb_ostream_t *stream, const pb_field_t *field, void * const *arg);
bool encode_metrics(pb_ostream_t *stream, const pb_field_t *field, void * const *arg);

#endif /* end of include guard: NODE_H */
