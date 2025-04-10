#ifndef SIMPLE_SNTP_H
#define SIMPLE_SNTP_H

#include "time.h"
#include <stdbool.h>

typedef struct {
    time_t now;
    struct tm timeinfo;
    char strftime_buf[64];
    bool synced;
} ntp_time;

void sntp_service_init(ntp_time* ntp_time);


#endif /* end of include guard: SIMPLE_SNTP_H */
