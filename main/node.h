#ifndef NODE_H
#define NODE_H

#include "mqtt_client.h"
#include "payload.h"
#include <time.h>


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
    NCMD_COUNT  // useful for bounds checking
} NCMDType;

typedef enum {
    MESSAGE_TYPE_UNKNOW = 0,
    NBIRTH,
    DBIRTH,
    NDATA,
    DDATA,
    DDEATH,
    NDEATH
} Message_Type;

// Topic namespace
// namespace/group_id/message_type/edge_node_id/[device_id]

const char* NCMDTypeToString(NCMDType type);
NCMDType StringToNCMDType(const char* str);

#define EN_NCMD_REBIRTH     (1U<<0)
#define EN_NCMD_REBOOT      (1U<<1)
#define EN_NCMD_SCAN_RATE   (1U<<2)
#define EN_NCMD_NEXT_SERVER (1U<<3)
typedef struct {
    const char* groupID;
    const char* nodeID;
    const char* namespace;

    char* Topic_NBIRTH;
    char* Topic_NDATA;
    char* Topic_NDEATH;

    int seq;
    int bdseq;
    uint8_t NCMD_bit_mark;
    Metrics *NBIRTH;
    sp_device devices[];
} Sparkplug_Node;


void Place_bdsep_Metric(sparkplug_payload_metric *bdseq, time_t* now);
void Place_NCMD_Metric(sparkplug_payload_metric *metric, time_t* now, NCMDType cmd);
void Place_Properties_HW_Model_Metric(sparkplug_payload_metric *rebirth, time_t* now, char* model);

/* Fill NCMD content into NBIRTH metric block */
void Node_Fill_NCMDs_NBIRTH_Metrics(Sparkplug_Node *node,time_t *now);
sparkplug_payload* Node_Auto_Generate_NBIRTH_payload(Sparkplug_Node *node);

/* Generate value for Topic_(Message type) field inside node and device */
void Node_Generate_Topic_Namespace(Sparkplug_Node *node, Message_Type msg_type);
#endif /* end of include guard: NODE_H */
