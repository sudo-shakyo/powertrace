#include "powertrace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#define PATTERN_FILE "patterns.conf"

void* run_collector(void* arg) {
    char patterns[10][64];
    int num_patterns = 0;
    FILE* pf = fopen(PATTERN_FILE, "r");
    if (pf) {
        while (fscanf(pf, "%63s", patterns[num_patterns]) == 1 && num_patterns < 10) {
            num_patterns++;
        }
        fclose(pf);
    } else {
        strcpy(patterns[0], "nvidia-drmdrmfb");
        num_patterns = 1;
    }

    int fd = open("/dev/kmsg", O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("Failed to open /dev/kmsg. Check permissions (sudo) or implement dmesg fallback");
        keep_running = 0;
        return NULL;
    }

    
    char junk[2048];
    while (read(fd, junk, sizeof(junk)) > 0);

    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;

    double last_event_ts = 0.0;
    char buffer[2048];
    
    while (keep_running) {
        if (poll(&pfd, 1, 1000) > 0) {
            int n = read(fd, buffer, sizeof(buffer) - 1);
            if (n > 0) {
                buffer[n] = '\0';
                for (int i = 0; i < num_patterns; i++) {
                    if (strstr(buffer, patterns[i])) {
                        double now = get_wall_time();
                        
                        
                        if (now - last_event_ts > 2.0) {
                            last_event_ts = now;
                            
                            struct disp_event *ev = malloc(sizeof(struct disp_event));
                            ev->ts = now;
                            snprintf(ev->type, sizeof(ev->type), "%.15s", patterns[i]);
                            strncpy(ev->msg, "Display disconnect matched", 255);
                            
                            
                            pthread_t corr_thread;
                            pthread_create(&corr_thread, NULL, run_correlator, ev);
                            pthread_detach(corr_thread);
                        }
                    }
                }
            }
        }
    }
    close(fd);
    return NULL;
}