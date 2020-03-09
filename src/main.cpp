#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include "configreader.h"
#include "process.h"

// Shared data for all cores
typedef struct SchedulerData {
    std::mutex mutex;
    std::condition_variable condition;
    ScheduleAlgorithm algorithm;
    uint32_t context_switch;
    uint32_t time_slice;
    std::list<Process*> ready_queue;
    bool all_terminated;
} SchedulerData;

void coreRunProcesses(uint8_t core_id, SchedulerData *data);
int printProcessOutput(std::vector<Process*>& processes, std::mutex& mutex);
void clearOutput(int num_lines);
uint32_t currentTime();
std::string processStateToString(Process::State state);

/* OS Scheduler Project
 * Nicholas Pumper and Yuki
 * 
 * 7 March 2020
 */

int main(int argc, char **argv)
{
    // ensure user entered a command line parameter for configuration file name
    if (argc < 2)
    {
        std::cerr << "Error: must specify configuration file" << std::endl;
        exit(1);
    }

    // declare variables used throughout main
    int i;
    SchedulerData *shared_data;
    std::vector<Process*> processes;

    // read configuration file for scheduling simulation
    SchedulerConfig *config = readConfigFile(argv[1]);

    // store configuration parameters in shared data object
    uint8_t num_cores = config->cores;
    shared_data = new SchedulerData();
    shared_data->algorithm = config->algorithm;
    shared_data->context_switch = config->context_switch;
    shared_data->time_slice = config->time_slice;
    shared_data->all_terminated = false;

    // create processes
    uint32_t start = currentTime();
	
    for (i = 0; i < config->num_processes; i++)
    {
        Process *p = new Process(config->processes[i], start);
        processes.push_back(p);
        if (p->getState() == Process::State::Ready)
        {
            shared_data->ready_queue.push_back(p);
        }
    } // for

    // free configuration data from memory
    deleteConfig(config);

    // launch 1 scheduling thread per cpu core
    std::thread *schedule_threads = new std::thread[num_cores];
    for (i = 0; i < num_cores; i++)
    {
        schedule_threads[i] = std::thread(coreRunProcesses, i, shared_data);
    }
    
	std::cout <<"Time: "<<start<<std::endl;
    
    // main thread work goes here:

    int num_lines = 0;
	int temp =0;        // flag for if we've reached the end of this loop at least once
    bool all_terminated = true;

    while (!(shared_data->all_terminated) && temp <1)
    {
        // clear output from previous iteration (Mirrinan)
        clearOutput(num_lines);

        // start new processes at their appropriate start time (Mirrinan)
    	for (i = 0; i < num_cores; i++)
    	{
		    std::cout<< "index: "<<i << std::endl;
        	schedule_threads[i].join();
    	}

	    //put ready processes to ready queue.
    	for (i = 0; i < processes.size(); i++)
    	{
		    //has to set state whether it is ready.
            if( processes[i]->getState() != Process::State::Ready ){
                //std::cout << processes[i]->getPid() << ": "<< processes[i]->getStartTime() <<": "<<start<< "\n";
                shared_data->ready_queue.push_back( processes[i] );
            }

    	}

        // determine when an I/O burst finishes and put the process back in the ready queue (Mirrinan)


        // sort the ready queue (Mirrinan)
        // NOTE: Round Robin and First Come First Serve DO NOT need to be sorted.
        // Shortest Job First
        if( shared_data->algorithm == ScheduleAlgorithm::SJF )
        { 
            shared_data->ready_queue.sort( SjfComparator() ); 
        }
        // Preemptive Priority
        if( shared_data->algorithm == ScheduleAlgorithm::PP )
        { 
            shared_data->ready_queue.sort( PpComparator() ); 
        }

        // determine if all processes are in the terminated state (Mirrinan)
        all_terminated = true;
        for (i = 0; i < processes.size(); i++)
        {
            //if at least one is not terminated, not all are not terminated.
            if( processes[i]->getState() != Process::State::Terminated )
            { 
                all_terminated = false; 
            }
        }

        if( all_terminated )
        {
            shared_data->all_terminated = true;
        }

        // output process status table (Mirrinan)
        num_lines = printProcessOutput(processes, shared_data->mutex);

        // sleep 1/60th of a second (Mirrinan)
        usleep(16667);
        temp++;
    }

    std::cout << "HERE!\n";

    // wait for threads to finish (Mirrinan)
    for (i = 0; i < num_cores; i++)
    {
        schedule_threads[i].join(); // (Mirrinan)
    } // for num cores
    
    // print final statistics
    //  - CPU utilization
    //  - Throughput
    //     - Average for first 50% of processes finished
    //     - Average for second 50% of processes finished
    //     - Overall average
    //  - Average turnaround time
    //  - Average waiting time

    // Clean up before quitting program 
    processes.clear(); // Mirrinan

    return 0;
}

void coreRunProcesses(uint8_t core_id, SchedulerData *shared_data)
{
    // Work to be done by each core idependent of the other cores
    //  - Get process at front of ready queue
    //  - Simulate the processes running until one of the following:
    //     - CPU burst time has elapsed
    //     - RR time slice has elapsed
    //     - Process preempted by higher priority process
    //  - Place the process back in the appropriate queue
    //     - I/O queue if CPU burst finished (and process not finished)
    //     - Terminated if CPU burst finished and no more bursts remain
    //     - Ready queue if time slice elapsed or process was preempted
    //  - Wait context switching time
    //  * Repeat until all processes in terminated state

    Process *p;
 	p = shared_data->ready_queue.front(); // get process at front of ready queue
 	p->setCpuCore(core_id);

  	shared_data->ready_queue.pop_front(); 

  	usleep( p->getStartTime() );
 	bool done = false;

    // simulate one of the processese until one of the following:
    //     - CPU burst time has elapsed
    //     - RR time slice has elapsed
    //     - Process preempted by higher priority process
 	std::unique_lock<std::mutex> lock(shared_data->mutex);
 	while( !done ){
 		p->updateProcess( currentTime() );
 		p->setState(Process::State::Running, p->getStartTime() );
 		done = true;

        // If the algo is Round Robin, see if the time slice has elapsed. 
        // if it has elapsed, then call it done.
        if (shared_data->algorithm == RR) {
            std::cout << "Time slice has elapsed! Booting from core for now.\n";

            
        } // if

        // if the algo is Preemptive priority, then boot if another process has a higher priority
        if (shared_data->algorithm == RR) {
            std::cout <<"Another process has higher priority. Booting from core for now.\n";

        } // if
 	} // while !done
 	lock.unlock();

    //  - Wait context switching time
    usleep( p->getWaitTime() );    

	/*
    std::cout <<  p->getPid() << ": ";
	std::cout <<  p->getStartTime() << ": ";
	std::cout <<  p->getState() << ": ";
	std::cout <<  p->getCpuCore() << std::endl;
	p->setState(Process::State::Terminated, 1000);
	shared_data->ready_queue.pop_front();*/

	std::cout <<  unsigned(core_id) << " : " << unsigned(p->getCpuCore()) << std::endl;
}

int printProcessOutput(std::vector<Process*>& processes, std::mutex& mutex) {
    int i;
    int num_lines = 2;
    std::lock_guard<std::mutex> lock(mutex);
    printf("|   PID | Priority |      State | Core | Turn Time | Wait Time | CPU Time | Remain Time |\n");
    printf("+-------+----------+------------+------+-----------+-----------+----------+-------------+\n");
    for (i = 0; i < processes.size(); i++)
    {
        if (processes[i]->getState() != Process::State::NotStarted)
        {
            uint16_t pid = processes[i]->getPid();
            uint8_t priority = processes[i]->getPriority();
            std::string process_state = processStateToString(processes[i]->getState());
            int8_t core = processes[i]->getCpuCore();
            std::string cpu_core = (core >= 0) ? std::to_string(core) : "--";
            double turn_time = processes[i]->getTurnaroundTime();
            double wait_time = processes[i]->getWaitTime();
            double cpu_time = processes[i]->getCpuTime();
            double remain_time = processes[i]->getRemainingTime();
            printf("| %5u | %8u | %10s | %4s | %9.1lf | %9.1lf | %8.1lf | %11.1lf |\n", 
                   pid, priority, process_state.c_str(), cpu_core.c_str(), turn_time, 
                   wait_time, cpu_time, remain_time);
            num_lines++;
        }
    }
    return num_lines;
}

void clearOutput(int num_lines)
{
    int i;
    for (i = 0; i < num_lines; i++)
    {
        fputs("\033[A\033[2K", stdout);
    }
    rewind(stdout);
    fflush(stdout);
}

uint32_t currentTime()
{
    uint32_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::system_clock::now().time_since_epoch()).count();
    return ms;
}

std::string processStateToString(Process::State state)
{
    std::string str;
    switch (state)
    {
        case Process::State::NotStarted:
            str = "not started";
            break;
        case Process::State::Ready:
            str = "ready";
            break;
        case Process::State::Running:
            str = "running";
            break;
        case Process::State::IO:
            str = "i/o";
            break;
        case Process::State::Terminated:
            str = "terminated";
            break;
        default:
            str = "unknown";
            break;
    }
    return str;
}
