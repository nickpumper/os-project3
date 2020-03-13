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

uint16_t Process::getPid() const
{
    return pid;
}

uint32_t Process::getStartTime() const
{
    return start_time;
}

uint8_t Process::getPriority() const
{
    return priority;
}

Process::State Process::getState() const
{
    return state;
}

int8_t Process::getCpuCore() const
{
    return core;
}

double Process::getTurnaroundTime() const
{
    return (double)turn_time / 1000.0;
}

double Process::getWaitTime() const
{
    return (double)wait_time / 1000.0;
}

double Process::getCpuTime() const
{
    return (double)cpu_time / 1000.0;
}

double Process::getRemainingTime() const
{
    return (double)remain_time / 1000.0;
}

int16_t Process::getCurrentBurst() const
{
	return current_burst;
}
int32_t Process::getBurstTime() const
{
	return burst_times[current_burst];
}
uint16_t Process::getNumBurst() const
{
	return num_bursts;
}

void Process::setState(State new_state, uint32_t current_time)
{
    if (state == State::NotStarted && new_state == State::Ready)
    {
        launch_time = current_time;
	previous_time = launch_time;
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
    uint32_t spent_time = ( current_time - previous_time );
	//std::cout << "previous time: " << previous_time <<" : " << launch_time << std::endl;
    if( spent_time < 0 ) { spent_time = 0; }
	//std::cout << "spent time" << spent_time << std::endl;
    // total time since 'launch' (until terminated)
    if( getState() != State::Terminated )
    {
    	turn_time = current_time - launch_time;
    }
    // total time spent in ready queue
    if( getState() == State::Ready )
    {
    	wait_time = wait_time + spent_time;
    }

    // total time spent running on a CPU core
    if( getState() == State::Running )
    {
    	cpu_time = cpu_time + spent_time;
    	// CPU time remaining until terminated
	remain_time = remain_time - spent_time;
    }
    //Track or update burst time in either running or IO.
    //odd numner: CPU bursts = I/O bursts + 1 ???
    if( getState() == State::Running || getState() == State::IO )
    {
	if( getBurstTime() < spent_time )
	{
		current_burst++;
	}
	else
	{
		updateBurstTime( current_burst, ( getBurstTime() -  spent_time ) );
	}
    }
	previous_time = current_time;
}//coreRunProcess

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
    return  p1->Process::getRemainingTime() < p2->Process::getRemainingTime(); // change this!
}

// PP - comparator for sorting read queue based on priority
bool PpComparator::operator ()(const Process *p1, const Process *p2)
{
    // your code here!
    return p1->Process::getPriority() != p2->Process::getPriority() ? p1->Process::getPriority() < p2->Process::getPriority() : false; // change this!
}
