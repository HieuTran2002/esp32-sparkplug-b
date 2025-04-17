#include "sparkplug_b.h"
#include "esp_heap_caps.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// General stuff
bool encode_string(pb_ostream_t *stream, const pb_field_t *field, void * const *arg){
    char* string = (char*)*arg;
    
    if (!pb_encode_tag_for_field(stream, field)) return false;
    if (!pb_encode_string(stream, (uint8_t*)string, strlen(string))) return false;
    return true;
}

bool encode_metrics(pb_ostream_t *stream, const pb_field_t *field, void * const *arg){
    Metrics *metrics = (Metrics*)*arg;

    // printf("merics %s %d\n", (char*)metrics->metrics->name.arg, metrics->used);

    for (int i=0; i<metrics->used; i++)
    {
        if (!pb_encode_tag_for_field(stream, field)) return false;


        if(!pb_encode_submessage(stream, org_eclipse_tahu_protobuf_Payload_Metric_fields, &metrics->metrics[i])){
            return false;
        }
    }
    return true;
}

/* -------------------- Metrics --------------------- */

/* Allocate new memory chunk for Metrics */
Metrics* Create_Metrics(uint8_t _capacity){
    Metrics *m = (Metrics*)malloc(sizeof(Metrics));
    if(!m) return NULL;
    m->capacity = _capacity;
    m->used = 0;
    m->metrics = (sparkplug_payload_metric*)malloc(sizeof(sparkplug_payload_metric) * _capacity);
    if(!m->metrics) {
        free(m);
        return NULL;
    }
    return m;
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

    sparkplug_payload_metric *metric = &m->metrics[m->used++];

    // Clean mem block
    memset(metric, 0, sizeof(*metric));
    return metric;
}

void free_metrics(Metrics* m){
    free(m->metrics);
    free(m);
}

/* -------------------- Metrics --------------------- */

/* -------------------- Message Type --------------------- */
const char* NCMDTypeToString(NCMDType type) {
    switch (type) {
        case NCMD_REBIRTH:       return "Node Control/Rebirth";
        case NCMD_REBOOT:        return "Node Control/Reboot";
        case NCMD_NEXT_SERVER:   return "Node Control/Next Server";
        case NCMD_SCAN_DEVICES:  return "Node Control/Scan Devices";
        case NCMD_SCAN_RATE:     return "Node Control/Scan Rate";
        case NCMD_DIAGNOSTICS:   return "Node Control/Diagnostics";
        case NCMD_ADD_METRIC:    return "Node Control/Add Metric";
        case NCMD_REMOVE_METRIC: return "Node Control/Remove Metric";
        default:                 return "Unknown";
    }
}


NCMDType StringToNCMDType(const char* str) {
    if (strcmp(str, "Node Control/Rebirth") == 0)       return NCMD_REBIRTH;
    if (strcmp(str, "Node Control/Reboot") == 0)        return NCMD_REBOOT;
    if (strcmp(str, "Node Control/Next Server") == 0)   return NCMD_NEXT_SERVER;
    if (strcmp(str, "Node Control/Scan Devices") == 0)  return NCMD_SCAN_DEVICES;
    if (strcmp(str, "Node Control/Diagnostics") == 0)   return NCMD_DIAGNOSTICS;
    if (strcmp(str, "Node Control/Add Metric") == 0)    return NCMD_ADD_METRIC;
    if (strcmp(str, "Node Control/Remove Metric") == 0) return NCMD_REMOVE_METRIC;
    return NCMD_UNKNOWN;
}

const char* MessageType_2_String(Message_Type msg_type){
    switch (msg_type) {
        case NBIRTH: return "NBIRTH";
        case DBIRTH: return "DBIRTH";
        case NDATA: return "NDATA";
        case DDATA: return "DDATA";
        case DDEATH: return "DDEATH";
        case NDEATH: return "NDEATH";
    
        default: return "UNKNOW";
    }
}

/* -------------------- Message Type --------------------- */


uint16_t Calculate_Topic_NS_Len(Sparkplug_Node *node, size_t msg_type_len, Sparkplug_Device *device){
    uint16_t len = strlen(node->namespace) + 1 
        + strlen(node->groupID) + 1
        + msg_type_len + 1
        + strlen(node->nodeID) + 2;


    if (device != NULL){
        len += strlen(device->deviceID) + 1;
    }

    return len;
}



/* -------------------- Sparkplug B Node -------------------- */

void Place_bdsep_Metric(sparkplug_payload_metric *bdseq, time_t* now){
    *bdseq = (sparkplug_payload_metric){
        .is_null = false,
        .has_timestamp = true,
        .timestamp = *now,
        .datatype = org_eclipse_tahu_protobuf_DataType_UInt32,
        .value.int_value = 0,
        .name.arg = "bdSeq",
        .name.funcs.encode = &encode_string,
        .which_value = org_eclipse_tahu_protobuf_Payload_Metric_int_value_tag,
        .has_datatype = true,
    };
}

void Place_NCMD_Metric(sparkplug_payload_metric *metric, time_t* now, NCMDType cmd){
    *metric = (sparkplug_payload_metric){
        .is_null = false,
        .has_timestamp = true,
        .timestamp = *now,
        .datatype = org_eclipse_tahu_protobuf_DataType_Boolean,
        .value.boolean_value = false,
        .name.arg = (void*)NCMDTypeToString(cmd),
        .name.funcs.encode = &encode_string,
        .which_value = org_eclipse_tahu_protobuf_Payload_Metric_boolean_value_tag,
        .has_datatype = true,
    };
    if (cmd == NCMD_SCAN_RATE){
        metric->datatype = org_eclipse_tahu_protobuf_DataType_UInt32;
        metric->which_value = org_eclipse_tahu_protobuf_Payload_Metric_int_value_tag;
        metric->value.int_value = 3000;
    }
}

void Place_Properties_HW_Model_Metric(sparkplug_payload_metric *hw_model, time_t* now, char* model){
    *hw_model = (sparkplug_payload_metric){
        .is_null = false,
            .has_timestamp = true,
            .timestamp = *now,
            .datatype = org_eclipse_tahu_protobuf_DataType_String,
            .value.string_value.arg = (void*)model,
            .value.string_value.funcs.encode = &encode_string,
            .name.arg = "Properties/Hardware Model",
            .name.funcs.encode = &encode_string,
            .which_value = org_eclipse_tahu_protobuf_Payload_Metric_string_value_tag,
            .has_datatype = true,
    };
}

void Node_Fill_NCMDs_NBIRTH_Metrics(Sparkplug_Node *node,time_t *now){
    sparkplug_payload_metric* p_metric = NULL;
    if(node->NCMD_bit_mark & EN_NCMD_SCAN_RATE){
        p_metric = (sparkplug_payload_metric*)add_metric(node->NBIRTH);
        if(p_metric) Place_NCMD_Metric(p_metric, now, NCMD_SCAN_RATE);
    }
    if(node->NCMD_bit_mark & EN_NCMD_REBOOT){
        p_metric = (sparkplug_payload_metric*)add_metric(node->NBIRTH);
        if(p_metric) Place_NCMD_Metric(p_metric, now, NCMD_REBOOT);
    }
    if(node->NCMD_bit_mark & EN_NCMD_REBIRTH){
        p_metric = (sparkplug_payload_metric*)add_metric(node->NBIRTH);
        if(p_metric) Place_NCMD_Metric(p_metric, now, NCMD_REBIRTH);
    }
}

/* Allocate heap mem for nbirth, fill content, return address of nbirth payload */
sparkplug_payload* Node_Auto_Generate_NBIRTH_payload(Sparkplug_Node *node){
    time_t now;
    time(&now);
    node->NBIRTH = Create_Metrics(4);
    Node_Fill_NCMDs_NBIRTH_Metrics(node, &now);

    // bdseq
    sparkplug_payload_metric *bdseq = add_metric(node->NBIRTH);
    Place_bdsep_Metric(bdseq, &now);

    sparkplug_payload* payload = (sparkplug_payload*)malloc(sizeof(sparkplug_payload));
    *payload = (sparkplug_payload){
        .metrics.arg = node->NBIRTH,
        .metrics.funcs.encode = &encode_metrics,
        .has_timestamp = 1,
        .timestamp = now,
        .seq = node->seq++,
        .has_seq = 1,
    };

    return payload;
}


/* Create topic path base on Node/Device information, used for both Node and Device */
void Generate_Topic_Namespace(Sparkplug_Node *node, Message_Type msg_type, Sparkplug_Device *device){
    const char* msg_type_string = MessageType_2_String(msg_type);

    uint16_t len = Calculate_Topic_NS_Len(node, strlen(msg_type_string), device);

    char* ns = NULL;
    switch (msg_type) {
        case NBIRTH:
            node->Topic_NBIRTH = (char*)malloc(len);
            ns = node->Topic_NBIRTH;
            break;

        case NDATA:
            node->Topic_NDATA = (char*)malloc(len);
            ns = node->Topic_NDATA;
            break;

        case NDEATH:
            node->Topic_NDEATH = (char*)malloc(len);
            ns = node->Topic_NDEATH;
            break;

        case DBIRTH:
            device->Topic_DBIRTH = (char*)malloc(len);
            ns = device->Topic_DBIRTH;
            break;
            
        case DDATA:
            device->Topic_DDATA = (char*)malloc(len);
            ns = device->Topic_DDATA;
            break;

        case DDEATH:
            device->Topic_DDEATH = (char*)malloc(len);
            ns = device->Topic_DDEATH;
            break;

        default:
            return;
    }

    if(device==NULL){
        snprintf(ns, len, "%s/%s/%s/%s", 
                node->namespace, 
                node->groupID, 
                msg_type_string, 
                node->nodeID);
    } else {
        snprintf(ns, len, "%s/%s/%s/%s/%s", 
                node->namespace, 
                node->groupID, 
                msg_type_string, 
                node->nodeID,
                device->deviceID);
    }
}

void Node_Generate_All_Topic_Namespace(Sparkplug_Node *node){
    Generate_Topic_Namespace(node, NBIRTH, NULL);
    Generate_Topic_Namespace(node, NDATA, NULL);
    Generate_Topic_Namespace(node, NDEATH, NULL);

    printf("NBIRTH: %s\n" , node->Topic_NBIRTH);
    printf("NDATA: %s\n" , node->Topic_NDATA);
    printf("NDEATH: %s\n" , node->Topic_NDEATH);
}

/**
 * Sparkplug B Device
 */
#define container_of(ptr, type, member)  ((type *)((char *)(ptr) - offsetof(type, member)))

void Device_Generate_All_Topic_Namespace(Sparkplug_Node *node, Sparkplug_Device *device){
    Generate_Topic_Namespace(node, DBIRTH, device);
    Generate_Topic_Namespace(node, DDATA, device);
    Generate_Topic_Namespace(node, DDEATH, device);

    printf("DBIRTH: %s\n", device->Topic_DBIRTH);
    printf("DDATA: %s\n", device->Topic_DDATA);
    printf("DDEATH: %s\n", device->Topic_DDEATH);
}
