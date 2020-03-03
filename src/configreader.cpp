#include "configreader.h"

SchedulerConfig* readConfigFile(const char *filename)
{
    std::string line;
    std::ifstream file(filename);
    SchedulerConfig *config = new SchedulerConfig();
    
    // read line 1 --> number of cpu cores
    std::getline(file, line);
    config->cores = std::stoi(line);

    // read line 2 --> scheduling algorithm
    std::getline(file, line);
    if      (line == "FCFS") config->algorithm = ScheduleAlgorithm::FCFS;
    else if (line == "SJF")  config->algorithm = ScheduleAlgorithm::SJF;
    else if (line == "RR")   config->algorithm = ScheduleAlgorithm::RR;
    else if (line == "PP")   config->algorithm = ScheduleAlgorithm::PP;

    // read line 3 --> context switch time (ms)
    std::getline(file, line);
    config->context_switch = std::stoi(line);

    // read line 4 --> time slice (ms)
    std::getline(file, line);
    config->time_slice = std::stoi(line);

    // read line 5 --> number of processes
    std::getline(file, line);
    config->num_processes = std::stoi(line);
    config->processes = new ProcessDetails[config->num_processes];

    // read lines 6 - N --> details for each process
    int i, j;
    std::string item1, item2;
    std::stringstream ss1, ss2;
    for (i = 0; i < config->num_processes; i++)
    {
        std::getline(file, line);
        ss1.clear();
        ss1.str(line);

        // column 1 --> pid
        std::getline(ss1, item1, ',');
        config->processes[i].pid = std::stoi(item1);

        // column 2 --> start time
        std::getline(ss1, item1, ',');
        config->processes[i].start_time = std::stoi(item1);

        // column 3 --> cpu and i/o burst times
        std::getline(ss1, item1, ',');
        config->processes[i].num_bursts = std::count(item1.begin(), item1.end(), '|') + 1;
        config->processes[i].burst_times = new uint32_t[config->processes[i].num_bursts];
        ss2.clear();
        ss2.str(item1);
        for (j = 0; j < config->processes[i].num_bursts; j++)
        {
            std::getline(ss2, item2, '|');
            config->processes[i].burst_times[j] = std::stoi(item2);
        }

        // column 4 --> priority
        std::getline(ss1, item1, ',');
        if (config->algorithm == ScheduleAlgorithm::PP)
        {
            config->processes[i].priority = std::stoi(item1);
        }
        else
        {
            config->processes[i].priority = 0;
        }
    }

    return config;
}

void deleteConfig(SchedulerConfig *config)
{
    int i;
    for (i = 0; i < config->num_processes; i++)
    {
        delete[] config->processes[i].burst_times;
    }
    delete[] config->processes;
    delete config;
    config = NULL;
}
