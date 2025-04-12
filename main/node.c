#include "node.h"
#include "pb_encode.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

bool encode_string(pb_ostream_t *stream, const pb_field_t *field, void * const *arg){
    char* string = (char*)*arg;
    
    if (!pb_encode_tag_for_field(stream, field)) return false;
    if (!pb_encode_string(stream, (uint8_t*)string, strlen(string))) return false;
    return true;
}

bool encode_metrics(pb_ostream_t *stream, const pb_field_t *field, void * const *arg){
    Metrics *metrics = (Metrics*)*arg;

    for (int i=0;i<metrics->used;i++)
    {
        if (!pb_encode_tag_for_field(stream, field)) return false;

        if(!pb_encode_submessage(stream, org_eclipse_tahu_protobuf_Payload_Metric_fields, &metrics->metrics[i])){
            return false;
        }
    }
    return true;
}

sparkplug_payload_metric *add_metric(Metrics *m){
    if(m->used >= m->capacity){
        if(!m->auto_expand) m->auto_expand = 1;
        size_t new_capacity = m->capacity + m->auto_expand;

        sparkplug_payload_metric* new_metrics = (sparkplug_payload_metric*)realloc(m->metrics, sizeof(sparkplug_payload_metric) * new_capacity);
        if(!new_metrics) return NULL;

        m->metrics = new_metrics;
        m->capacity = new_capacity;
    }

    return &m->metrics[m->used++];
}

void free_metrics(Metrics* m){
    free(m->metrics);
    free(m);
}
