#ifndef SPARKPLUG_H
#define SPARKPLUG_H

#include "sparkplug_b.pb.h"
#include "pb_encode.h"
#include "pb.h"
#include <stdint.h>
#include <time.h>


/* -------------------- Macros --------------------- */
#define sparkplug_payload org_eclipse_tahu_protobuf_Payload
#define sparkplug_payload_metric org_eclipse_tahu_protobuf_Payload_Metric


/* -------------------- Type Definition for message and payload --------------------- */
typedef enum {
    NCMD_UNKNOWN = 0,
    NCMD_REBIRTH,
    NCMD_REBOOT,
    NCMD_NEXT_SERVER,
    NCMD_SCAN_DEVICES,
    NCMD_SCAN_RATE,
    NCMD_DIAGNOSTICS,
    NCMD_ADD_METRIC,
    NCMD_REMOVE_METRIC,
} NCMDType;

typedef enum {
    DCMD_UNKNOWN = 0,
    DCMD_REBIRTH,
    DCMD_REBOOT,
    DCMD_SCAN_RATE,
} DCMDType;

typedef enum {
    MESSAGE_TYPE_UNKNOW = 0,
    NBIRTH,
    DBIRTH,
    NDATA,
    DDATA,
    NCMD,
    DCMD,
    DDEATH,
    NDEATH
} Message_Type;

typedef struct{
    uint8_t used;
    uint8_t capacity;
    uint8_t auto_expand;
    sparkplug_payload_metric *metrics;
} Metrics;

typedef struct{
    uint8_t Count;
    Metrics** Mul_Metrics_ptrs;
} Stack_Metrics_ptrs;


/**
 * Structure to hold data related to Sparkplug_b device.
 *  - All 'Metrics' aren't meant to be frequestly modify.
 *
 * Description.
 */
typedef struct {
    const char* deviceID;

    char* Topic_DBIRTH;
    char* Topic_DCMD;
    char* Topic_DDATA;
    char* Topic_DDEATH;

    uint8_t DCMD_bit_mark;

    Stack_Metrics_ptrs *DBIRTH; // DBIRTH is consist of DDATA, DCMD, and Properties
    Metrics *DCMD;
    Metrics *DDATA;
    Metrics *Properties;
    Metrics *DDEATH;
} Sparkplug_Device;

typedef struct {
    const char* groupID;
    const char* nodeID;
    const char* namespace;

    char* Topic_NBIRTH;
    char* Topic_NCMD;
    char* Topic_NDATA;
    char* Topic_NDEATH;

    int seq;
    int bdseq;
    uint8_t NCMD_bit_mark;
    Metrics *NBIRTH;
    Metrics *NDATA;
    Metrics *NCMD;
    Metrics *NDEATH;
    Sparkplug_Device devices[];
} Sparkplug_Node;

/* -------------------- Payload + Message --------------------- */

const char* NCMDTypeToString(NCMDType type);
NCMDType StringToNCMDType(const char* str);
const char* MessageType_2_String(Message_Type msg_type);

bool encode_string(pb_ostream_t *stream, const pb_field_t *field, void * const *arg);

/**
 * Encode one 'Metrics'.
 *  - Pass *Metrics to sparkplug_payload.metrics.arg
 *
 * Description.
 */
bool encode_metrics(pb_ostream_t *stream, const pb_field_t *field, void * const *arg);

bool encode_multiple_metrics_ptr(pb_ostream_t *stream, const pb_field_t *field, void * const *arg);

Metrics* Create_Metrics(uint8_t _capacity);
sparkplug_payload_metric *add_metric(Metrics *m);
void free_metrics(Metrics* m);
uint16_t Calculate_Topic_NS_Len(Sparkplug_Node *node, size_t msg_type_len, Sparkplug_Device *device);

/* -------------------- Encoded buffer --------------------- */
typedef struct {
    uint8_t buffer[255];
    uint16_t buffer_len;
    size_t encoded_length;
} Encode_Buffer;

bool encode_payload(Encode_Buffer *ebuffer, sparkplug_payload *payload);

/* --------------------- Sparkplug Node -------------------- */
const char* NCMDTypeToString(NCMDType type);
NCMDType StringToNCMDType(const char* str);

#define EN_NCMD_REBIRTH     (1U<<0)
#define EN_NCMD_REBOOT      (1U<<1)
#define EN_NCMD_SCAN_RATE   (1U<<2)
#define EN_NCMD_NEXT_SERVER (1U<<3)

void Place_bdsep_Metric(sparkplug_payload_metric *bdseq, time_t* now);
void Place_NCMD_Metric(sparkplug_payload_metric *metric, time_t* now, NCMDType cmd);
void Place_Properties_HW_Model_Metric(sparkplug_payload_metric *rebirth, time_t* now, char* model);

/* Fill NCMD content into NBIRTH metric block */
void Node_Fill_NCMDs_NBIRTH_Metrics(Sparkplug_Node *node,time_t *now);
sparkplug_payload* Node_Auto_Generate_NBIRTH_payload(Sparkplug_Node *node);

/* Generate value for Topic_(Message type) field inside node and device */
void Generate_Topic_Namespace(Sparkplug_Node *node, Message_Type msg_type, Sparkplug_Device *device);
void Node_Generate_All_Topic_Namespace(Sparkplug_Node *node);

/* -------------------- Sparkplug Device --------------------- */
void Device_Generate_All_Topic_Namespace(Sparkplug_Node *node, Sparkplug_Device *device);
void Device_Fill_DCMDs_Metrics(Sparkplug_Device *device,time_t *now);
void free_device(Sparkplug_Device *device);

#define EN_DCMD_REBIRTH     (1U<<0)
#define EN_DCMD_REBOOT      (1U<<1)
#define EN_DCMD_SCAN_RATE   (1U<<2)

#endif /* end of include guard: SPARKPLUG_H */
