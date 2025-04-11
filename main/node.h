#ifndef NODE_H
#define NODE_H

#include "sparkplug_b.pb.h"

#define NAMESPACE "spBv1.0"

#define sparkplug_payload_metric org_eclipse_tahu_protobuf_Payload_Metric
#define sparkplug_payload org_eclipse_tahu_protobuf_Payload

typedef struct {
    const char* deviceID;
    sparkplug_payload metrics[];
} sp_device;

typedef struct {
    const char* groupID;
    const char* nodeID;
    int seq;
    int bdseq;
    sp_device devices[];
} sp_node;

typedef struct{
    size_t size;
    org_eclipse_tahu_protobuf_Payload_Metric metrics[];
} Metrics;

bool encode_string(pb_ostream_t *stream, const pb_field_t *field, void * const *arg);
bool encode_metrics(pb_ostream_t *stream, const pb_field_t *field, void * const *arg);

#endif /* end of include guard: NODE_H */
