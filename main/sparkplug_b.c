#include "sparkplug_b.h"
#include "esp_heap_caps.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// General stuff
bool encode_payload(Encode_Buffer *ebuffer, sparkplug_payload *payload){
    pb_ostream_t stream = pb_ostream_from_buffer(ebuffer->buffer, ebuffer->buffer_len);
    ebuffer->encoded_length = 0;
    if(!pb_encode(&stream, org_eclipse_tahu_protobuf_Payload_fields, payload))
        return false;

    ebuffer->encoded_length = stream.bytes_written;
    return true;
}


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

/**
 * Encode multiple 'Metrics' at once, use with sparkplug_payload.metrics.funcs.encode.
 *  -Only pass &Stack_Metrics_ptrs to sparkplug_payload.metrics.arg.
 */
bool encode_multiple_metrics_ptr(pb_ostream_t *stream, const pb_field_t *field, void * const *arg){
    Stack_Metrics_ptrs *stack_metrics = (Stack_Metrics_ptrs*)*arg;

    // printf("merics %s %d\n", (char*)metrics->metrics->name.arg, metrics->used);

    for (int i=0; i<stack_metrics->Count; i++)
    {
        for(int j=0; j < stack_metrics->Mul_Metrics_ptrs[i]->used; j++){

            // printf("%s\n", (char*)stack_metrics->Mul_Metrics_ptrs[i]->metrics[j].name.arg);
            
            // Insert field
            if (!pb_encode_tag_for_field(stream, field)) return false;

            // encode metric
            if(!pb_encode_submessage(stream, org_eclipse_tahu_protobuf_Payload_Metric_fields, &stack_metrics->Mul_Metrics_ptrs[i]->metrics[j])){
                return false;
            }
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

/**
 * Return the pointer after Metrics.used, if not enough space, allocate more.
 */
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
    if(m->metrics){ free(m->metrics); m->metrics = NULL; }
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

const char* DCMDTypeToString(DCMDType type) {
    switch (type) {
        case DCMD_REBIRTH:       return "Device Control/Rebirth";
        case DCMD_REBOOT:        return "Device Control/Reboot";
        case DCMD_SCAN_RATE:     return "Device Control/Scan Rate";
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
        case NBIRTH:    return "NBIRTH";
        case DBIRTH:    return "DBIRTH";
        case NDATA:     return "NDATA";
        case NCMD:      return "NCMD";
        case DCMD:      return "DCMD";
        case DDATA:     return "DDATA";
        case DDEATH:    return "DDEATH";
        case NDEATH:    return "NDEATH";
    
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

void Node_Fill_NCMDs_Metrics(Sparkplug_Node *node,time_t *now){
    sparkplug_payload_metric* p_metric = NULL;
    if(node->NCMD_bit_mark & EN_NCMD_SCAN_RATE){
        p_metric = (sparkplug_payload_metric*)add_metric(node->NCMD);
        if(p_metric) Place_NCMD_Metric(p_metric, now, NCMD_SCAN_RATE);
    }
    if(node->NCMD_bit_mark & EN_NCMD_REBOOT){
        p_metric = (sparkplug_payload_metric*)add_metric(node->NCMD);
        if(p_metric) Place_NCMD_Metric(p_metric, now, NCMD_REBOOT);
    }
    if(node->NCMD_bit_mark & EN_NCMD_REBIRTH){
        p_metric = (sparkplug_payload_metric*)add_metric(node->NCMD);
        if(p_metric) Place_NCMD_Metric(p_metric, now, NCMD_REBIRTH);
    }
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

        case NCMD:
            node->Topic_NCMD = (char*)malloc(len);
            ns = node->Topic_NCMD;
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

        case DCMD:
            device->Topic_DCMD = (char*)malloc(len);
            ns = device->Topic_DCMD;
            break;

        case DDEATH:
            device->Topic_DDEATH = (char*)malloc(len);
            ns = device->Topic_DDEATH;
            break;

        default:
            return;
    }

    if(device==NULL){
        snprintf(ns, len, "%s/%s/%s/%s", node->namespace, node->groupID, msg_type_string, node->nodeID);
    } else {
        snprintf(ns, len, "%s/%s/%s/%s/%s", node->namespace, node->groupID, msg_type_string, node->nodeID, device->deviceID);
    }
}

void Node_Generate_All_Topic_Namespace(Sparkplug_Node *node){
    Generate_Topic_Namespace(node, NBIRTH, NULL);
    Generate_Topic_Namespace(node, NDATA, NULL);
    Generate_Topic_Namespace(node, NDEATH, NULL);
    Generate_Topic_Namespace(node, NCMD, NULL);

    printf("NBIRTH: %s\n" , node->Topic_NBIRTH);
    printf("NDATA: %s\n" , node->Topic_NDATA);
    printf("NDEATH: %s\n" , node->Topic_NDEATH);
    printf("NCMD: %s\n" , node->Topic_NCMD);
}


/* ------------------------ Device Manager -------------------------- */
Device_Manager_t *create_device_manager(uint8_t cap){
    Device_Manager_t *dm = (Device_Manager_t*)malloc(sizeof(Device_Manager_t) + sizeof(Sparkplug_Device) * cap);

    dm->capacity = cap;
    dm->used = 0;
    return dm;
}

void add_device(Device_Manager_t* dm, Sparkplug_Device *d){
    if(dm->capacity <= dm->used){
        uint8_t new_cap = dm->capacity + AUTO_EXPAND;
        dm->devices = realloc(dm->devices, sizeof(Sparkplug_Device*) * new_cap);
        if(!dm->devices){
            // handle allocation failure
            perror("Failed to realloc devices array");
            return;
        }
        dm->capacity = new_cap;
    }
    dm->devices[dm->used++] = d;
}


/* ----------------------------- Sparkplug Device ------------------------------- */

void Device_Generate_All_Topic_Namespace(Sparkplug_Node *node, Sparkplug_Device *device){
    Generate_Topic_Namespace(node, DBIRTH, device);
    Generate_Topic_Namespace(node, DDATA, device);
    Generate_Topic_Namespace(node, DDEATH, device);
    Generate_Topic_Namespace(node, DCMD, device);

    printf("DBIRTH: %s\n", device->Topic_DBIRTH);
    printf("DDATA: %s\n", device->Topic_DDATA);
    printf("DDEATH: %s\n", device->Topic_DDEATH);
    printf("DCMD: %s\n", device->Topic_DCMD);
}

void Place_DCMD_Metric(sparkplug_payload_metric *metric, time_t* now, DCMDType cmd){
    *metric = (sparkplug_payload_metric){
        .is_null = false,
        .has_timestamp = true,
        .timestamp = *now,
        .datatype = org_eclipse_tahu_protobuf_DataType_Boolean,
        .value.boolean_value = false,
        .name.arg = (void*)DCMDTypeToString(cmd),
        .name.funcs.encode = &encode_string,
        .which_value = org_eclipse_tahu_protobuf_Payload_Metric_boolean_value_tag,
        .has_datatype = true,
    };
    if (cmd == DCMD_SCAN_RATE){
        metric->datatype = org_eclipse_tahu_protobuf_DataType_UInt32;
        metric->which_value = org_eclipse_tahu_protobuf_Payload_Metric_int_value_tag;
        metric->value.int_value = 3000;
    }
}
void Device_Fill_DCMDs_Metrics(Sparkplug_Device *device,time_t *now){
    sparkplug_payload_metric* p_metric = NULL;

    if(device->DCMD_bit_mark & EN_DCMD_SCAN_RATE){
        p_metric = (sparkplug_payload_metric*)add_metric(device->DCMD);
        if(p_metric) Place_DCMD_Metric(p_metric, now, DCMD_SCAN_RATE);
    }
    if(device->DCMD_bit_mark & EN_DCMD_REBOOT){
        p_metric = (sparkplug_payload_metric*)add_metric(device->DCMD);
        if(p_metric) Place_DCMD_Metric(p_metric, now, DCMD_REBOOT);
    }
    if(device->DCMD_bit_mark & EN_DCMD_REBIRTH){
        p_metric = (sparkplug_payload_metric*)add_metric(device->DCMD);
        if(p_metric) Place_DCMD_Metric(p_metric, now, DCMD_REBIRTH);
    }
}


void free_Stack_Metrics_ptrs(Stack_Metrics_ptrs *c) {
    if (!c) return;

    if (c->Mul_Metrics_ptrs) {
        for (size_t i = 0; i < c->Count; ++i) {
            if (c->Mul_Metrics_ptrs[i]) {
                free_metrics(c->Mul_Metrics_ptrs[i]);
                c->Mul_Metrics_ptrs[i] = NULL;  // Avoid dangling
            }
        }
        free(c->Mul_Metrics_ptrs);   // Free the array of pointers
        c->Mul_Metrics_ptrs = NULL;
    }

    free(c);  // Free the container itself
}

// Free all allocated fields
void free_device(Sparkplug_Device *d){
    if(!d) return;
    free_Stack_Metrics_ptrs(d->DBIRTH);

    // free allocated metrics
    if(d->DCMD) {free_metrics(d->DCMD); d->DCMD = NULL; printf("free 1\n");}
    if(d->DDATA) {free_metrics(d->DDATA); d->DDATA = NULL; printf("free 2\n");}
    if(d->Properties) {free_metrics(d->Properties); d->Properties = NULL; printf("free 4\n");}
    if(d->DDEATH) {free_metrics(d->DDEATH); d->DDEATH = NULL; printf("free 3\n");}


    // free allocated string
    if(d->Topic_DBIRTH) {free(d->Topic_DBIRTH); d->Topic_DBIRTH=NULL;}
    if(d->Topic_DDATA)  {free(d->Topic_DDATA);  d->Topic_DDATA=NULL;}
    if(d->Topic_DCMD)   {free(d->Topic_DCMD);   d->Topic_DCMD=NULL;}
    if(d->Topic_DDEATH) {free(d->Topic_DDEATH); d->Topic_DDEATH=NULL;}


    free(d);
}

void free_device_manager(Device_Manager_t *dm){
    for(int i=0; i < dm->used; i++){
        free(dm->devices[i]);
        dm->devices[i] = NULL;
    }
    free(dm);
}

void free_node(Sparkplug_Node *n){
    if(!n) return;
    if(n->NBIRTH) {free_Stack_Metrics_ptrs(n->NBIRTH); n->NBIRTH = NULL; printf("free 0\n");}

    if(n->NCMD) {free_metrics(n->NCMD); n->NCMD = NULL; printf("free 1\n");}
    if(n->NDATA) {free_metrics(n->NDATA); n->NDATA = NULL; printf("free 2\n");}
    if(n->NDEATH) {free_metrics(n->NDEATH); n->NDEATH = NULL; printf("free 3\n");}
    if(n->Properties) {free_metrics(n->Properties); n->Properties = NULL; printf("free 4\n");}

    if(n->Topic_NBIRTH) {free(n->Topic_NBIRTH); n->Topic_NBIRTH=NULL;}
    if(n->Topic_NDATA)  {free(n->Topic_NDATA);  n->Topic_NDATA=NULL;}
    if(n->Topic_NCMD)   {free(n->Topic_NCMD);   n->Topic_NCMD=NULL;}
    if(n->Topic_NDEATH) {free(n->Topic_NDEATH); n->Topic_NDEATH=NULL;}

    if(n->Device_Manager) free_device_manager(n->Device_Manager);
}
