#ifndef POWERTRACE_H
#define POWERTRACE_H

#include <pthread.h>

#define RING_BUFFER_SIZE 60

struct disp_event { 
    double ts; 
    char type[16]; 
    char msg[256]; 
};

struct power_sample { 
    double ts; 
    int ac_online; 
    long voltage_uv; 
    long current_ua; 
    char status[16]; 
};

struct verdict { 
    double ts; 
    int class; 
    double score; 
    char reason[128]; 
};

extern struct power_sample power_ring[RING_BUFFER_SIZE];
extern int ring_head;
extern pthread_mutex_t ring_mutex;
extern volatile int keep_running;

void* run_collector(void* arg);
void* run_sampler(void* arg);
void* run_correlator(void* arg);
void reporter_init();
void report_verdict(struct verdict v);
void generate_summary();
double get_wall_time();

#endif