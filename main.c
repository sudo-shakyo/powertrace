#include "powertrace.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

volatile int keep_running = 1;
pthread_mutex_t ring_mutex = PTHREAD_MUTEX_INITIALIZER;
struct power_sample power_ring[RING_BUFFER_SIZE];
int ring_head = 0;

void intHandler(int dummy) {
    keep_running = 0;
}

int main() {
    signal(SIGINT, intHandler);
    signal(SIGTERM, intHandler);
    
    pthread_t sampler_thread, collector_thread;
    
    printf("Starting PowerTrace Daemon...\n");
    reporter_init();
    
    pthread_create(&sampler_thread, NULL, run_sampler, NULL);
    pthread_create(&collector_thread, NULL, run_collector, NULL);
    
    pthread_join(collector_thread, NULL);
    pthread_join(sampler_thread, NULL);
    
    generate_summary();
    printf("PowerTrace has stopped safely. \n");
    return 0;
}
