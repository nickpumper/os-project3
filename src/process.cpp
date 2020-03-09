#include "process.h"

// Process class methods
Process::Process(ProcessDetails details, uint32_t current_time)
{
    int i;
    pid = details.pid;
    start_time = details.start_time;
    num_bursts = details.num_bursts;
    current_burst = 0;
    burst_times = new uint32_t[num_bursts];
    for (i = 0; i < num_bursts; i++)
    {
        burst_times[i] = details.burst_times[i];
    }
    priority = details.priority;
    state = (start_time == 0) ? State::Ready : State::NotStarted;
    if (state == State::Ready)
    {
        launch_time = current_time;
    }
    core = -1;
    turn_time = 0;
    wait_time = 0;
    cpu_time = 0;
    remain_time = 0;
    for (i = 0; i < num_bursts; i+=2)
    {
        remain_time += burst_times[i];
    }
}

Process::~Process()
{
    delete[] burst_times;
}

uint16_t Process::getPid()
{
    return pid;
}

uint32_t Process::getStartTime()
{
    return start_time;
}

uint8_t Process::getPriority()
{
    return priority;
}
//create new const func
uint8_t Process::getPriority_const() const
{
    return priority;
}

Process::State Process::getState()
{
    return state;
}

int8_t Process::getCpuCore()
{
    return core;
}

double Process::getTurnaroundTime()
{
    return (double)turn_time / 1000.0;
}

double Process::getWaitTime()
{
    return (double)wait_time / 1000.0;
}

double Process::getCpuTime()
{
    return (double)cpu_time / 1000.0;
}

double Process::getRemainingTime()
{
    return (double)remain_time / 1000.0;
}
//create new const func
double Process::getRemainingTime_const() const
{
    return (double)remain_time / 1000.0;
}

void Process::setState(State new_state, uint32_t current_time)
{
    if (state == State::NotStarted && new_state == State::Ready)
    {
        launch_time = current_time;
    }
    state = new_state;
}

void Process::setCpuCore(int8_t core_num)
{
    core = core_num;
}

void Process::updateProcess(uint32_t current_time)
{
    // use `current_time` to update turnaround time, wait time, burst times, 
    // cpu time, and remaining time
	int i;
	std::cout << "HERE: "<<current_time << ": "<< turn_time <<std::endl;

    turn_time = turn_time + current_time/1000000.0; // total time since 'launch' (until terminated)
    wait_time = wait_time + ( current_time/1000000.0 - start_time ) ; // total time spent in ready queue
	std::cout << "turn_time: "<< turn_time <<std::endl;
	std::cout << "wait_time: "<< wait_time <<std::endl;
    // CPU/IO burst array of times (in ms)
	uint32_t total_burst;
    for (i = 0; i < num_bursts; i++)
    {
        //burst_times[i] = 0;
	total_burst += burst_times[i];
    }
	std::cout << "HERE2: "<< total_burst <<std::endl;
    cpu_time = cpu_time + turn_time - wait_time - total_burst/1000000.0; // total time spent running on a CPU core
    // CPU time remaining until terminated
    for (i = 0; i < num_bursts; i+=2)
    {
        remain_time += burst_times[i];
    }
}

void Process::updateBurstTime(int burst_idx, uint32_t new_time)
{
    burst_times[burst_idx] = new_time;
}

// Comparator methods: used in std::list sort() method
// No comparator needed for FCFS or RR (ready queue never sorted)
// SJF - comparator for sorting read queue based on shortest remaining CPU time
bool SjfComparator::operator ()(const Process *p1, const Process *p2)
{
    // your code here!
    return  p1->getRemainingTime_const() < p2->getRemainingTime_const(); // change this!
}

// PP - comparator for sorting read queue based on priority
bool PpComparator::operator ()(const Process *p1, const Process *p2)
{
    // your code here!
    return p1->getPriority_const() < p2->getPriority_const(); // change this!
}
