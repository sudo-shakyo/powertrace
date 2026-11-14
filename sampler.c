#include "powertrace.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>

double get_wall_time() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void find_ac_path(char* path_buffer, size_t max_len) {
    DIR *d = opendir("/sys/class/power_supply/");
    struct dirent *dir;
    
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            
            if (strstr(dir->d_name, "AC") || strstr(dir->d_name, "ADP")) {
                snprintf(path_buffer, max_len, "/sys/class/power_supply/%s/online", dir->d_name);
                printf("[Sampler] Dynamically bound to power sensor: %s\n", dir->d_name);
                closedir(d);
                return;
            }
        }
        closedir(d);
    }
    
    strncpy(path_buffer, "/sys/class/power_supply/AC/online", max_len);
}

void* run_sampler(void* arg) {
    char ac_online_path[256];
    find_ac_path(ac_online_path, sizeof(ac_online_path));
    
    while(keep_running) {
        struct power_sample s;
        s.ts = get_wall_time();
        
        FILE* f = fopen(ac_online_path, "r");
        if (f) {
            fscanf(f, "%d", &s.ac_online);
            fclose(f);
        } else {
            s.ac_online = -1; 
        }
        
        s.voltage_uv = 0; 
        s.current_ua = 0;
        strcpy(s.status, "Unknown");
        
        pthread_mutex_lock(&ring_mutex);
        power_ring[ring_head] = s;
        ring_head = (ring_head + 1) % RING_BUFFER_SIZE;
        pthread_mutex_unlock(&ring_mutex);
        
        usleep(100000); 
    }
    return NULL;
}