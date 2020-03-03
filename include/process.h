#ifndef __PROCESS_H_
#define __PROCESS_H_

#include "configreader.h"

// Process class
class Process {
public:
    enum State : uint8_t {NotStarted, Ready, Running, IO, Terminated};

private:
    uint16_t pid;             // process ID
    uint32_t start_time;      // ms after program starts that process should be 'launched'
    uint16_t num_bursts;      // number of CPU/IO bursts
    uint16_t current_burst;   // current index into the CPU/IO burst array
    uint32_t *burst_times;    // CPU/IO burst array of times (in ms)
    uint8_t priority;         // process priority (0-4)
    State state;              // process state
    int8_t core;              // CPU core currently running on
    int32_t turn_time;        // total time since 'launch' (until terminated)
    int32_t wait_time;        // total time spent in ready queue
    int32_t cpu_time;         // total time spent running on a CPU core
    int32_t remain_time;      // CPU time remaining until terminated
    uint32_t launch_time;     // actual time in ms (since epoch) that process was 'launched'
    // you are welcome to add other private data fields here (e.g. actual time process was put in 
    // ready queue or i/o queue)

public:
    Process(ProcessDetails details, uint32_t current_time);
    ~Process();

    uint16_t getPid();
    uint32_t getStartTime();
    uint8_t getPriority();
    State getState();
    int8_t getCpuCore();
    double getTurnaroundTime();
    double getWaitTime();
    double getCpuTime();
    double getRemainingTime();

    void setState(State new_state, uint32_t current_time);
    void setCpuCore(int8_t core_num);

    void updateProcess(uint32_t current_time);
    void updateBurstTime(int burst_idx, uint32_t new_time);
};

// Comparators: used in std::list sort() method
// No comparator needed for FCFS or RR (ready queue never sorted)
struct SjfComparator {
    bool operator ()(const Process *p1, const Process *p2);
};

struct PpComparator {
    bool operator ()(const Process *p1, const Process *p2);
};

#endif // __PROCESS_H_