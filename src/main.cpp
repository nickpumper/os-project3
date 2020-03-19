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
    }

    // free configuration data from memory
    deleteConfig(config);

    // launch 1 scheduling thread per cpu core
    std::thread *schedule_threads = new std::thread[num_cores];
    for (i = 0; i < num_cores; i++)
    {
        schedule_threads[i] = std::thread(coreRunProcesses, i, shared_data);
    }

    // main thread work goes here:
    int num_lines = 0;
    bool all_terminated = true;
    uint16_t previous_burst = 0;
    while ( !(shared_data->all_terminated) )
    {
        // clear output from previous iteration
        clearOutput(num_lines);

        std::unique_lock<std::mutex> lock(shared_data->mutex);
        all_terminated = true;
    	for (i = 0; i < processes.size(); i++)
    	{
	    previous_burst = processes[i]->getCurrentBurst();

            // start new processes at their appropriate start time
            processes[i]->updateProcess( currentTime() );

            //if at least one is not terminated, not all are not terminated.
            if( processes[i]->getState() != Process::State::Terminated )
            {
            	//put process to ready queue if a process is time to lunch.
            	if( processes[i]->getState() == Process::State::NotStarted &&
             	       				processes[i]->getStartTime()+start <= currentTime() )
            	{
            	    processes[i]->setState( Process::State::Ready, currentTime() );
            	    shared_data->ready_queue.push_back( processes[i] );
            	}
                // determine when an I/O burst finishes and put the process back in the ready queue
            	if( processes[i]->getState() == Process::State::IO &&
            	            			previous_burst < processes[i]->getCurrentBurst() )
            	{
            	    processes[i]->setState( Process::State::Ready, currentTime() );
            	    shared_data->ready_queue.push_back( processes[i] );
            	}
            	all_terminated = false;
            }//
    	}//for
        // sort the ready queue (if needed - based on scheduling algorithm)
        if( shared_data->algorithm == ScheduleAlgorithm::SJF )
        { 
       		shared_data->ready_queue.sort( SjfComparator() );
        }
        if( shared_data->algorithm == ScheduleAlgorithm::PP )
        { 
        	shared_data->ready_queue.sort( PpComparator() );
        }
        shared_data->all_terminated = all_terminated;
        lock.unlock();
        shared_data->condition.notify_one();

        // output process status table
        num_lines = printProcessOutput(processes, shared_data->mutex);

        // sleep 1/60th of a second
        usleep(16667);
    } // while

    // wait for threads to finish
    for (i = 0; i < num_cores; i++)
    {
        schedule_threads[i].join();
    }

    // print final statistics
    //  - CPU utilization
    //  - Throughput
    //     - Average for first 50% of processes finished
    //     - Average for second 50% of processes finished
    //     - Overall average
    //  - Average turnaround time
    //  - Average waiting time

    int cpuUtilization = 0; 
    int avgThroughputFirstHalf = 0;
    int avgThroughputLastHalf =0;
    int totalAvgThroughput = 0;
    double avgTurnaroundTime = 0;
    double avgWaitingTime = 0;

    double cpuTimeSum = 0;
    double turnaroundSum = 0;
    double waitSum = 0;
    double numProcesses = processes.size();

    uint32_t totalTime = (currentTime() - start) / 1000; // get total CPU time, convert from ms to seconds

    // compute stastics when we pop each process off the vector
    while (!processes.empty()) {
        // fetch and pop off of the vector
        Process * process = processes.back();
        processes.pop_back();

        waitSum = waitSum + process->getWaitTime();
        turnaroundSum = turnaroundSum + process->getTurnaroundTime();
    }

    avgTurnaroundTime = turnaroundSum / numProcesses;
    avgWaitingTime = waitSum / numProcesses;
    cpuUtilization = (cpuTimeSum / totalTime) * 100; // THIS IS W

    std::cout<< "\n------ SIMULATION COMPLETE -----\n\nSimulation Statistics:\n";
    std::cout<<"CPU Utilization: " << cpuUtilization <<"\n"; // THIS WILL NOT RETURN THE CORRECT VALUE AT THE MOMENT 
    std::cout<<"Throughput Averages:\n";
    std::cout<<"\tAverage for first 50% of processes finished: " << avgThroughputFirstHalf << "\n";
    std::cout<<"\tAverage for second 50% of processes finished: " << avgThroughputLastHalf << "\n";
    std::cout<<"\tOverall Throughput Average"<< totalAvgThroughput << "\n";
    std::cout<<"Average Turnaround Time: " << avgTurnaroundTime << "\n";
    std::cout<<"Average Waiting Time: " << avgWaitingTime << "\n";
    std::cout <<"\n";

    // Clean up before quitting program
    processes.clear();

    return 0;
} // main

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
	uint32_t start_cpu_time;
	int32_t cpu_burst_time;
	uint16_t previous_burst;
	Process *p;
	bool pri;

    	while ( !(shared_data->all_terminated) )
	{
		p = NULL;
		start_cpu_time = 0;
		previous_burst = 0;
		cpu_burst_time = 0;
		pri = false;

    		//  - Get process at front of ready queue
		if( p == NULL && !(shared_data->all_terminated) )
		{
			if( !shared_data->ready_queue.empty() )
			{
                		std::unique_lock<std::mutex> lock(shared_data->mutex);
                		p = shared_data->ready_queue.front();
                		shared_data->ready_queue.pop_front();
                		p->setCpuCore(core_id);
                		p->setState( Process::State::Running, currentTime() );

				cpu_burst_time = p->getBurstTime();
               			previous_burst = p->getCurrentBurst();

                		p->updateProcess( currentTime() );
				start_cpu_time = currentTime();

                		lock.unlock();
                		shared_data->condition.notify_one();
			}
		} // if( p == NULL && !(shared_data->all_terminated)

        	//  - Simulate the processes running until one of the following:
        	//     - CPU burst time has elapsed
        	//     - RR time slice has elapsed
        	//     - Process preempted by higher priority process
		if( p!=NULL )
		{
			if( p->getState() == Process::State::Running )
			{
                		if( shared_data->algorithm == ScheduleAlgorithm::RR )
                		{
               			     while( shared_data->time_slice > (currentTime() - start_cpu_time) );
                		}
                		else if( shared_data->algorithm == ScheduleAlgorithm::PP )
                		{
				    pri = false;//lower numner is priority.
				    while( currentTime()  < start_cpu_time + cpu_burst_time && !pri )
				    {
					std::unique_lock<std::mutex> lock3(shared_data->mutex);
					if( shared_data->ready_queue.front() != NULL &&
						shared_data->ready_queue.front()->getPriority() < p->getPriority() )
					{ 
						pri = true;
					}
                			lock3.unlock();
                			shared_data->condition.notify_one();
				    }
                		}
                		else
                		{
                    			while( currentTime()  < start_cpu_time + cpu_burst_time );
                		}
			}//running
		}//null

        	//  - Place the process back in the appropriate queue
        	//  - I/O queue if CPU burst finished (and process not finished)
        	//  - Terminated if CPU burst finished and no more bursts remain
        	//  - Ready queue if time slice elapsed or process was preempted
		if( p!=NULL )
		{
			std::unique_lock<std::mutex> lock2(shared_data->mutex);
			p->setCpuCore(-1);
			p->updateProcess( currentTime() );
        		//  - I/O queue if CPU burst finished (and process not finished)
			if( previous_burst < p->getCurrentBurst() && p->getRemainingTime() > 0 )
			{
				p->setState( Process::State::IO, currentTime() );
			}
        		//  - Terminated if CPU burst finished and no more bursts remain
			else if( previous_burst < p->getCurrentBurst() && p->getRemainingTime() <= 0 )
			{
				p->setState( Process::State::Terminated, currentTime() );
			}
			//  - Ready queue if time slice elapsed or process was preempted
			else
			{
                		p->setState( Process::State::Ready, currentTime() );
                		shared_data->ready_queue.push_back( p );
			}
			lock2.unlock();
			shared_data->condition.notify_one();
		}//p!=NULL

    		//  - Wait context switching time
		usleep( shared_data->context_switch );

	} // !(shared_data->all_terminated)
} /// runCoreProcesses

int printProcessOutput(std::vector<Process*>& processes, std::mutex& mutex)
{
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
            uint8_t priority = processes[i]->Process::getPriority();
            std::string process_state = processStateToString(processes[i]->getState());
            int8_t core = processes[i]->getCpuCore();
            std::string cpu_core = (core >= 0) ? std::to_string(core) : "--";
            double turn_time = processes[i]->getTurnaroundTime();
            double wait_time = processes[i]->getWaitTime();
            double cpu_time = processes[i]->getCpuTime();
            double remain_time = processes[i]->Process::getRemainingTime();
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
