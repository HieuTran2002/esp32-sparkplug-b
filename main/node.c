#include "node.h"
#include "pb_encode.h"

bool encode_string(pb_ostream_t *stream, const pb_field_t *field, void * const *arg){
    char* string = (char*)*arg;
    
    if (!pb_encode_tag_for_field(stream, field)) return false;
    if (!pb_encode_string(stream, (uint8_t*)string, strlen(string))) return false;
    return true;
}

bool encode_metrics(pb_ostream_t *stream, const pb_field_t *field, void * const *arg){
    Metrics *metrics = (Metrics*)*arg;

    for (int i=0;i<metrics->size;i++)
    {
        if (!pb_encode_tag_for_field(stream, field)) return false;

        if(!pb_encode_submessage(stream, org_eclipse_tahu_protobuf_Payload_Metric_fields, &metrics->metrics[i])){
            return false;
        }
    }
    return true;
}
