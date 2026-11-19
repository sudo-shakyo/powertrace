#include "powertrace.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

void* run_correlator(void* arg) {
    struct disp_event* ev = (struct disp_event*)arg;
    struct verdict v;
    v.ts = ev->ts;
    

    if (strcmp(ev->type, "AC_DROP") == 0) {
        v.class = 0;
        v.score = 0.99;
        strcpy(v.reason, "Direct AC voltage drop detected");
        report_verdict(v);
        free(ev);
        return NULL;
    }


    sleep(5); 
    
    pthread_mutex_lock(&ring_mutex);
    int power_issue_found = 0;
    for(int i = 0; i < RING_BUFFER_SIZE; i++) {
        if (power_ring[i].ts > 0 && 
            (power_ring[i].ts >= ev->ts - 5.0) && 
            (power_ring[i].ts <= ev->ts + 5.0)) {

            if (power_ring[i].ac_online == 0) {
                power_issue_found = 1;
                break;
            }
        }
    }
    pthread_mutex_unlock(&ring_mutex);
    
    if (power_issue_found) {
        v.class = 0;
        v.score = 0.85;
        strcpy(v.reason, "Voltage changed near display drop");
    } else {
        v.class = 1;
        v.score = 0.90;
        strcpy(v.reason, "Power stable throughout window");
    }
    
    report_verdict(v);
    free(ev); 
    return NULL;
}