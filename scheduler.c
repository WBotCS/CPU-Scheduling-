#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Headers as needed

typedef enum {false, true} bool;        // Allows boolean types in C

/* Defines a job struct */
typedef struct Process {
    uint32_t A;                         // A: Arrival time of the process
    uint32_t B;                         // B: Upper Bound of CPU burst times of the given random integer list
    uint32_t C;                         // C: Total CPU time required
    uint32_t M;                         // M: Mult~iplier of CPU burst time
    uint32_t processID;                 // The process ID given upon input read

    uint8_t status;                     // 0 is unstarted, 1 is ready, 2 is running, 3 is blocked, 4 is terminated

    int32_t finishingTime;              // The cycle when the the process finishes (initially -1)
    uint32_t currentCPUTimeRun;         // The amount of time the process has already run (time in running state)
    uint32_t currentIOBlockedTime;      // The amount of time the process has been IO blocked (time in blocked state)
    uint32_t currentWaitingTime;        // The amount of time spent waiting to be run (time in ready state)

    uint32_t IOBurst;                   // The amount of time until the process finishes being blocked
    uint32_t CPUBurst;                  // The CPU availability of the process (has to be > 1 to move to running)

    int32_t quantum;                    // Used for schedulers that utilise pre-emption

    bool isFirstTimeRunning;            // Used to check when to calculate the CPU burst when it hits running mode

    struct Process* nextInBlockedList;  // A pointer to the next process available in the blocked list
    struct Process* nextInReadyQueue;   // A pointer to the next process available in the ready queue
    struct Process* nextInReadySuspendedQueue; // A pointer to the next process available in the ready suspended queue
} _process;


uint32_t CURRENT_CYCLE = 0;             // The current cycle that each process is on
uint32_t TOTAL_CREATED_PROCESSES = 0;   // The total number of processes constructed
uint32_t TOTAL_STARTED_PROCESSES = 0;   // The total number of processes that have started being simulated
uint32_t TOTAL_FINISHED_PROCESSES = 0;  // The total number of processes that have finished running
uint32_t TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED = 0; // The total cycles in the blocked state

const char* RANDOM_NUMBER_FILE_NAME= "random-numbers";
const uint32_t SEED_VALUE = 200;  // Seed value for reading from file

// Additional variables as needed
uint32_t IO_UTILIZED_CYCLES = 0;

/**
 * Reads a random non-negative integer X from a file with a given line named random-numbers (in the current directory)
 */
uint32_t getRandNumFromFile(uint32_t line, FILE* random_num_file_ptr){
    uint32_t end, loop;
    char str[512];

    rewind(random_num_file_ptr); // reset to be beginning
    for(end = loop = 0;loop<line;++loop){
        if(0==fgets(str, sizeof(str), random_num_file_ptr)){ //include '\n'
            end = 1;  //can't input (EOF)
            break;
        }
    }
    if(!end) {
        return (uint32_t) atoi(str);
    }

    // fail-safe return
    return (uint32_t) 1804289383;
}



/**
 * Reads a random non-negative integer X from a file named random-numbers.
 * Returns the CPU Burst: : 1 + (random-number-from-file % upper_bound)
 */
uint32_t randomOS(uint32_t upper_bound, uint32_t process_indx, FILE* random_num_file_ptr)
{
    char str[20];
    
    uint32_t unsigned_rand_int = (uint32_t) getRandNumFromFile(SEED_VALUE+process_indx, random_num_file_ptr);
    uint32_t returnValue = 1 + (unsigned_rand_int % upper_bound);

    return returnValue;
} 


/********************* SOME PRINTING HELPERS *********************/


/**
 * Prints to standard output the original input
 * process_list is the original processes inputted (in array form)
 */
void printStart(_process process_list[])
{
    printf("The original input was: %i", TOTAL_CREATED_PROCESSES);

    uint32_t i = 0;
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        printf(" ( %i %i %i %i)", process_list[i].A, process_list[i].B,
               process_list[i].C, process_list[i].M);
    }
    printf("\n");
} 

/**
 * Prints to standard output the final output
 * finished_process_list is the terminated processes (in array form) in the order they each finished in.
 */
void printFinal(_process finished_process_list[])
{
    printf("The (sorted) input is: %i", TOTAL_CREATED_PROCESSES);

    uint32_t i = 0;
    for (; i < TOTAL_FINISHED_PROCESSES; ++i)
    {
        printf(" ( %i %i %i %i)", finished_process_list[i].A, finished_process_list[i].B,
               finished_process_list[i].C, finished_process_list[i].M);
    }
    printf("\n");
} // End of the print final function

/**
 * Prints out specifics for each process.
 * @param process_list The original processes inputted, in array form
 */
void printProcessSpecifics(_process process_list[])
{
    uint32_t i = 0;
    printf("\n");
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        printf("Process %i:\n", process_list[i].processID);
        printf("\t(A,B,C,M) = (%i,%i,%i,%i)\n", process_list[i].A, process_list[i].B,
               process_list[i].C, process_list[i].M);
        printf("\tFinishing time: %i\n", process_list[i].finishingTime);
        printf("\tTurnaround time: %i\n", process_list[i].finishingTime - process_list[i].A);
        printf("\tI/O time: %i\n", process_list[i].currentIOBlockedTime);
        printf("\tWaiting time: %i\n", process_list[i].currentWaitingTime);
        printf("\n");
    }
} // End of the print process specifics function

/**
 * Prints out the summary data
 * process_list The original processes inputted, in array form
 */
void printSummaryData(_process process_list[])
{
    uint32_t i = 0;
    double total_amount_of_time_utilizing_cpu = 0.0;
    double total_amount_of_time_io_blocked = 0.0;
    double total_amount_of_time_spent_waiting = 0.0;
    double total_turnaround_time = 0.0;
    uint32_t final_finishing_time = CURRENT_CYCLE - 1;
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        total_amount_of_time_utilizing_cpu += process_list[i].currentCPUTimeRun;
        total_amount_of_time_io_blocked += process_list[i].currentIOBlockedTime;
        total_amount_of_time_spent_waiting += process_list[i].currentWaitingTime;
        total_turnaround_time += (process_list[i].finishingTime - process_list[i].A);
    }

    //debugging segment fault
    if (final_finishing_time == 0) {
    printf("Error: final_finishing_time is zero!\n");
    return; 
    }

    // Calculates the CPU utilisation
    double cpu_util = total_amount_of_time_utilizing_cpu / final_finishing_time;

    // Calculates the IO utilisation
    double io_util = (double) TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED / final_finishing_time;
    //
    // double comiO =(double) IO_UTILIZED_CYCLES / CURRENT_CYCLE;

    // Calculates the throughput (Number of processes over the final finishing time times 100)
    double throughput =  100 * ((double) TOTAL_CREATED_PROCESSES/ final_finishing_time);

    // Calculates the average turnaround time
    double avg_turnaround_time = total_turnaround_time / TOTAL_CREATED_PROCESSES;

    // Calculates the average waiting time
    double avg_waiting_time = total_amount_of_time_spent_waiting / TOTAL_CREATED_PROCESSES;

    //debugging
    // printf("Debug: IO_UTILIZED_CYCLES = %d\n", IO_UTILIZED_CYCLES);
    // printf("Debug: CURRENT_CYCLE = %d\n", CURRENT_CYCLE);

    printf("Summary Data:\n");
    printf("\tFinishing time: %i\n", CURRENT_CYCLE - 1);
    printf("\tCPU Utilisation: %6f\n", cpu_util);
    printf("\tI/O Utilisation: %6f\n", io_util);
    // printf("\tI/O Utilisation: %6f\n", comiO);
    printf("\tThroughput: %6f processes per hundred cycles\n", throughput);
    printf("\tAverage turnaround time: %6f\n", avg_turnaround_time);
    printf("\tAverage waiting time: %6f\n", avg_waiting_time);
} // End of the print summary data function

// ** Additional Helpers:

// reset all processes after each run.
void resetProcesses(_process process_list[]) {
    for (int i = 0; i < TOTAL_CREATED_PROCESSES; i++) {
        process_list[i].status = 0; // unstarted
        process_list[i].finishingTime = -1;
        process_list[i].currentCPUTimeRun = 0;
        process_list[i].currentIOBlockedTime = 0;
        process_list[i].currentWaitingTime = 0;
        process_list[i].IOBurst = 0;
        process_list[i].CPUBurst = 0;
        process_list[i].quantum = 0;
        process_list[i].isFirstTimeRunning = true;
        process_list[i].nextInBlockedList = NULL;
        process_list[i].nextInReadyQueue = NULL;
        process_list[i].nextInReadySuspendedQueue = NULL;
    }
}

// Helper function to find the index of the shortest job in the ready queue
uint32_t shortestJobIndex(_process *readyQueue[], uint32_t count) {
    if (count == 0) return -1;

    uint32_t shortestTimeIndex = 0;
    for (uint32_t i = 1; i < count; i++) {
        if (readyQueue[i]->C - readyQueue[i]->currentCPUTimeRun < readyQueue[shortestTimeIndex]->C - readyQueue[shortestTimeIndex]->currentCPUTimeRun) {
            shortestTimeIndex = i;
        }
    }
    return shortestTimeIndex;
}

// Helper function to move a process from one queue to another
void moveProcessToQueue(_process **sourceQueue, uint32_t *sourceCount, _process **destQueue, uint32_t *destCount, uint32_t index) {
    if (destQueue) {
        destQueue[*destCount] = sourceQueue[index];
        (*destCount)++;
    }
    for (uint32_t j = index; j < *sourceCount - 1; j++) {
        sourceQueue[j] = sourceQueue[j + 1];
    }
    (*sourceCount)--;
}

void moveToReadyQueue(_process **queue, uint32_t *queueCount, _process *process) {
    queue[*queueCount] = process;
    (*queueCount)++;
}

_process* dequeueFromReadyQueue(_process **queue, uint32_t *queueCount) {
    _process* proc = queue[0];
    for (uint32_t i = 0; i < *queueCount - 1; i++) {
        queue[i] = queue[i + 1];
    }
    (*queueCount)--;
    return proc;
}

void executeFCFS(_process process_list[], FILE* random_num_file_ptr) {

    printf("######################### START OF FIRST COME FIRST SERVE #########################\n");
    
    printStart(process_list);

    
    _process *readyQueue[TOTAL_CREATED_PROCESSES];
    uint32_t readyQueueCount = 0;

    _process *blockedQueue[TOTAL_CREATED_PROCESSES];
    uint32_t blockedQueueCount = 0;

    _process *runningProcess = NULL;

    while(TOTAL_FINISHED_PROCESSES < TOTAL_CREATED_PROCESSES) {
        // Check for arriving processes
        for (uint32_t i = 0; i < TOTAL_CREATED_PROCESSES; i++) {
            if (process_list[i].A == CURRENT_CYCLE) {
                process_list[i].status = 1;  // Ready
                readyQueue[readyQueueCount++] = &process_list[i];
            }
        }

        // Check for process to run
        if (runningProcess == NULL && readyQueueCount > 0) {
            runningProcess = readyQueue[0];
            // Shift ready queue
            for (uint32_t i = 0; i < readyQueueCount - 1; i++) {
                readyQueue[i] = readyQueue[i + 1];
            }
            readyQueueCount--;
            runningProcess->status = 2;  // Running
            runningProcess->CPUBurst = randomOS(runningProcess->B, runningProcess->processID, random_num_file_ptr);
        }

        // Execute the running process
        if (runningProcess) {
            runningProcess->currentCPUTimeRun++;
            runningProcess->CPUBurst--;

            // If CPU burst is done
            if (runningProcess->CPUBurst == 0) {
                uint32_t previousCPUBurst = randomOS(runningProcess->B, runningProcess->processID, random_num_file_ptr);

                if (runningProcess->currentCPUTimeRun == runningProcess->C) {
                    // Process is done
                    runningProcess->status = 4;  // Terminated
                    runningProcess->finishingTime = CURRENT_CYCLE;
                    TOTAL_FINISHED_PROCESSES++;
                    runningProcess = NULL;
                } else {
                    // Move to blocked for I/O
                    runningProcess->status = 3;  // Blocked
                    runningProcess->IOBurst = runningProcess->M * previousCPUBurst;
                    runningProcess->currentIOBlockedTime += runningProcess->IOBurst; // Increase IO time
                    blockedQueue[blockedQueueCount++] = runningProcess;
                    runningProcess = NULL;
                }
            }
        }

        // Increase waiting time for processes in the ready queue
        for (uint32_t i = 0; i < readyQueueCount; i++) {
            readyQueue[i]->currentWaitingTime++;
        }

        // Handle blocked processes
        for (uint32_t i = 0; i < blockedQueueCount; i++) {
            blockedQueue[i]->IOBurst--;
            if (blockedQueue[i]->IOBurst == 0) {
                // Move back to ready queue
                blockedQueue[i]->status = 1;  // Ready
                readyQueue[readyQueueCount++] = blockedQueue[i];
                // Shift blocked queue
                for (uint32_t j = i; j < blockedQueueCount - 1; j++) {
                    blockedQueue[j] = blockedQueue[j + 1];
                }
                blockedQueueCount--;
                i--;
            }
        }

        CURRENT_CYCLE++;
    }
    
    printFinal(process_list);
    printf("\n");
    printf("The scheduling algorithm used was First Come First Serve\n");
    printProcessSpecifics(process_list);
    printSummaryData(process_list);
    printf("######################### END OF FIRST COME FIRST SERVE #########################\n");
}


void executeRR(_process process_list[], FILE* random_num_file_ptr) {
    IO_UTILIZED_CYCLES = 0;
    const uint32_t QUANTUM = 2;

    printf("######################### START OF ROUND ROBIN #########################\n");

    printStart(process_list);

    _process *readyQueue[TOTAL_CREATED_PROCESSES];
    uint32_t readyQueueCount = 0;

    _process *blockedQueue[TOTAL_CREATED_PROCESSES];
    uint32_t blockedQueueCount = 0;

    _process *runningProcess = NULL;
    uint32_t quantumCounter = 0;

    while (TOTAL_FINISHED_PROCESSES < TOTAL_CREATED_PROCESSES) {
        // printf("Cycle %d: \n", CURRENT_CYCLE);

        // Check for arriving processes
        for (uint32_t i = 0; i < TOTAL_CREATED_PROCESSES; i++) {
            if (process_list[i].A == CURRENT_CYCLE) {
                // printf("Process %d has arrived.\n", i);
                process_list[i].status = 1;  // Ready
                moveToReadyQueue(readyQueue, &readyQueueCount, &process_list[i]);
            }
        }

        // If no process is running and ready queue is not empty, start a new one
        if (!runningProcess && readyQueueCount > 0) {
            runningProcess = dequeueFromReadyQueue(readyQueue, &readyQueueCount);
            quantumCounter = 0;
            // printf("Starting to run Process %d.\n", runningProcess->processID);
            if (runningProcess->isFirstTimeRunning) {
                runningProcess->CPUBurst = randomOS(runningProcess->B, runningProcess->processID, random_num_file_ptr);
                runningProcess->isFirstTimeRunning = false;
            }
        }

        // Execute the running process
        if (runningProcess) {
            quantumCounter++;
            runningProcess->currentCPUTimeRun++;
            runningProcess->CPUBurst--;

            if (runningProcess->currentCPUTimeRun == runningProcess->C) {
                // printf("Process %d has finished.\n", runningProcess->processID);
                runningProcess->status = 4;  // Terminated
                runningProcess->finishingTime = CURRENT_CYCLE;
                TOTAL_FINISHED_PROCESSES++;
                runningProcess = NULL;
                quantumCounter = 0;
            } else if (runningProcess->CPUBurst == 0 || quantumCounter == QUANTUM) {
                if (runningProcess->CPUBurst == 0) {
                    // printf("Process %d is blocked.\n", runningProcess->processID);
                    runningProcess->status = 3;  // Blocked
                    runningProcess->IOBurst = runningProcess->M * randomOS(runningProcess->B, runningProcess->processID, random_num_file_ptr);
                    runningProcess->currentIOBlockedTime += runningProcess->IOBurst; // Increment the I/O time for the process
                    blockedQueue[blockedQueueCount++] = runningProcess;
                } else {
                    moveToReadyQueue(readyQueue, &readyQueueCount, runningProcess); // Requeue if time quantum expires
                }
                runningProcess = NULL;
                quantumCounter = 0;
            }
        }

        // Handle blocked processes
        for (uint32_t i = 0; i < blockedQueueCount; i++) {
            blockedQueue[i]->IOBurst--;
            blockedQueue[i]->currentIOBlockedTime++; // Increment the I/O time for every blocked cycle
            if (blockedQueue[i]->IOBurst == 0) {
                // printf("Blocked Process %d is now ready.\n", blockedQueue[i]->processID);
                moveToReadyQueue(readyQueue, &readyQueueCount, blockedQueue[i]);
                moveProcessToQueue(blockedQueue, &blockedQueueCount, NULL, NULL, i);
                i--;
            }
        }
        if (blockedQueueCount>0) {
            IO_UTILIZED_CYCLES++;
        }

        CURRENT_CYCLE++;
    }

    printFinal(process_list);
    printf("\n");
    printf("The scheduling algorithm used was Round Robin\n");
    printProcessSpecifics(process_list);
    printSummaryData(process_list);
    printf("######################### END OF ROUND ROBIN #########################\n");
}



void executeSJF(_process process_list[], FILE* random_num_file_ptr) {

    IO_UTILIZED_CYCLES = 0;
    printf("######################### START OF SHORTEST JOB FIRST #########################\n");

    printStart(process_list);

    _process *readyQueue[TOTAL_CREATED_PROCESSES];
    uint32_t readyQueueCount = 0;

    _process *blockedQueue[TOTAL_CREATED_PROCESSES];
    uint32_t blockedQueueCount = 0;

    _process *runningProcess = NULL;

    while (TOTAL_FINISHED_PROCESSES < TOTAL_CREATED_PROCESSES) {
        // printf("Cycle %d: \n", CURRENT_CYCLE);

        // Check for arriving processes
        for (uint32_t i = 0; i < TOTAL_CREATED_PROCESSES; i++) {
            if (process_list[i].A == CURRENT_CYCLE) {
                // printf("Process %d has arrived.\n", i); //
                process_list[i].status = 1;  // Ready
                readyQueue[readyQueueCount++] = &process_list[i];
            }
        }

        // If no process is running, start the shortest job
        if (!runningProcess && readyQueueCount > 0) {
            uint32_t shortestTimeIndex = shortestJobIndex(readyQueue, readyQueueCount);
            runningProcess = readyQueue[shortestTimeIndex];
            // printf("Starting to run Process %d.\n", runningProcess->processID); //
            moveProcessToQueue(readyQueue, &readyQueueCount, NULL, NULL, shortestTimeIndex);
            runningProcess->status = 2;  // Running
            if (runningProcess->isFirstTimeRunning) {
                runningProcess->CPUBurst = randomOS(runningProcess->B, runningProcess->processID, random_num_file_ptr);
                runningProcess->isFirstTimeRunning = false;
            }
        }

        // Execute the running process
        if (runningProcess) {
            runningProcess->currentCPUTimeRun++;
            runningProcess->CPUBurst--;

            if (runningProcess->currentCPUTimeRun == runningProcess->C) {
                // printf("Process %d has finished.\n", runningProcess->processID);//
                runningProcess->status = 4;  // Terminated
                runningProcess->finishingTime = CURRENT_CYCLE;
                TOTAL_FINISHED_PROCESSES++;
                runningProcess = NULL;
            } else if (runningProcess->CPUBurst == 0) {
                // printf("Process %d is blocked.\n", runningProcess->processID);//
                runningProcess->status = 3;  // Blocked
                runningProcess->IOBurst = runningProcess->M * randomOS(runningProcess->B, runningProcess->processID, random_num_file_ptr);
                runningProcess->currentIOBlockedTime += runningProcess->IOBurst; // Increment the I/O time for the process
                runningProcess->currentCPUTimeRun = 0;  // Reset current CPU run time
                blockedQueue[blockedQueueCount++] = runningProcess;
                runningProcess = NULL;
            }
        }

        // Handle blocked processes
        for (uint32_t i = 0; i < blockedQueueCount; i++) {
            blockedQueue[i]->IOBurst--;
            blockedQueue[i]->currentIOBlockedTime++; // Increment the I/O time for every blocked cycle
            if (blockedQueue[i]->IOBurst == 0) {
                // printf("Blocked Process %d is now ready.\n", blockedQueue[i]->processID); //
                blockedQueue[i]->status = 1;  // Ready
                blockedQueue[i]->CPUBurst = 0;  // Reset CPU burst
                readyQueue[readyQueueCount++] = blockedQueue[i];
                moveProcessToQueue(blockedQueue, &blockedQueueCount, NULL, NULL, i);
                i--;
            }
        }
        if (blockedQueueCount>0)
        {
            IO_UTILIZED_CYCLES++;
        }

        CURRENT_CYCLE++;
    }

    printFinal(process_list);
    printf("\n");
    printf("The scheduling algorithm used was Shortest Job First\n");
    printProcessSpecifics(process_list);
    printSummaryData(process_list);
    printf("######################### END OF SHORTEST JOB FIRST #########################\n");
}



int main(int argc, char *argv[])
{
    // Open file pointer to read random numbers
    FILE* randomNumFile = fopen(RANDOM_NUMBER_FILE_NAME, "r");

    // Read input file 
    //printf("File name: %s\n", argv[1]);
    FILE* inputFile = fopen(argv[1], "r");
    if(!inputFile) {
        printf("Error opening input file\n");
        return 1;
    }

    // Read number of processes
    fscanf(inputFile, "%i", &TOTAL_CREATED_PROCESSES);

    // Allocate process list
    _process* process_list = (_process*)malloc(TOTAL_CREATED_PROCESSES * sizeof(_process));

    // Read process info and initialize
    for(int i=0; i<TOTAL_CREATED_PROCESSES; i++) {
        fscanf(inputFile, " %*c%i %i %i %i%*c", &process_list[i].A, &process_list[i].B, &process_list[i].C, &process_list[i].M);
        process_list[i].processID = i;
        process_list[i].status = 0; // unstarted
        process_list[i].finishingTime = -1;
        process_list[i].currentCPUTimeRun = 0;
        process_list[i].currentIOBlockedTime = 0;
        process_list[i].currentWaitingTime = 0;
        process_list[i].IOBurst = 0;
        process_list[i].CPUBurst = 0;
        process_list[i].quantum = 0;
        process_list[i].isFirstTimeRunning = true;
        process_list[i].nextInBlockedList = NULL;
        process_list[i].nextInReadyQueue = NULL;
        process_list[i].nextInReadySuspendedQueue = NULL;
    }
    
    CURRENT_CYCLE = 0;
    TOTAL_FINISHED_PROCESSES = 0;
    executeFCFS(process_list, randomNumFile);

    resetProcesses(process_list);

    CURRENT_CYCLE = 0;
    TOTAL_FINISHED_PROCESSES = 0;
    executeRR(process_list, randomNumFile);

    resetProcesses(process_list);

    CURRENT_CYCLE = 0;
    TOTAL_FINISHED_PROCESSES = 0;
    executeSJF(process_list, randomNumFile);





    fclose(inputFile);
    fclose(randomNumFile);



    return 0;
} 