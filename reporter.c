#include "powertrace.h"
#include <stdio.h>
#include <time.h>

static int total_dropouts = 0;
static int power_related = 0;
static int hardware_related = 0;

void reporter_init() {
    FILE* f = fopen("powertrace_log.csv", "w");
    if (f) {
        fprintf(f, "Timestamp,Class,Score,Reason,Power_Fault,Adapter_Fault\n");
        fclose(f);
    }
}

void report_verdict(struct verdict v) {
    
    time_t raw_time = (time_t)v.ts;
    struct tm * timeinfo = localtime(&raw_time);
    char time_buffer[80];
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

   
    const char* power_fault = (v.class == 0) ? "Yes" : "No";
    const char* adapter_fault = (v.class == 1) ? "Yes" : "No";

    FILE* f = fopen("powertrace_log.csv", "a");
    if (f) {
        fprintf(f, "%s,%d,%.2f,%s,%s,%s\n", 
                time_buffer, v.class, v.score, v.reason, power_fault, adapter_fault);
        fclose(f);
    }
    total_dropouts++;
    if (v.class == 0) power_related++;
    else if (v.class == 1) hardware_related++;
    
    printf("[Reporter] %s - New Event Logged: Class %d (Reason: %s)\n", time_buffer, v.class, v.reason);
}

void generate_summary() {
    printf("\n--- PowerTrace Final Report ---\n");
    printf("Total Dropouts: %d\n", total_dropouts);
    printf("Power-related: %d\n", power_related);
    printf("Likely Hardware: %d\n", hardware_related);
    
    if (hardware_related > 0 && power_related == 0) {
        printf("Verdict: Stable power. Your adapter/cable is likely failing.\n");
    } else if (power_related > 0) {
        printf("Verdict: Power fluctuations detected. Adapter is likely fine.\n");
    } else {
        printf("Verdict: Inconclusive. Not enough data.\n");
    }
}