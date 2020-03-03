#ifndef __CONFIGREADER_H_
#define __CONFIGREADER_H_

#include <iostream>
#include <string>
#include <algorithm>
#include <fstream>
#include <sstream>

enum ScheduleAlgorithm : uint8_t { FCFS, SJF, RR, PP };

typedef struct ProcessDetails {
    uint16_t pid;
    uint32_t start_time;
    uint16_t num_bursts;
    uint32_t *burst_times;
    uint8_t priority;
} ProcessDetails;

typedef struct SchedulerConfig {
    uint8_t cores;
    ScheduleAlgorithm algorithm;
    uint32_t context_switch;
    uint32_t time_slice;
    uint16_t num_processes;
    ProcessDetails *processes;
} SchedulerConfig;

SchedulerConfig* readConfigFile(const char *filename);
void deleteConfig(SchedulerConfig *config);

#endif // __CONFIGREADER_H_