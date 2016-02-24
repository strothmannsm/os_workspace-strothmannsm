#ifndef _PROCESS_SCHEDULING_H_
#define _PROCESS_SCHEDULING_H_
#include <dyn_array.h>
#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>

typedef struct {
	uint32_t remaining_burst_time; // the remaining burst of the pcb
	uint32_t started; // first activated on virtual CPU
} ProcessControlBlock_t;

typedef struct {
	float average_latency_time; // the average waiting time in the ready queue until first schedue on the cpu
	float average_wall_clock_time; // the average completion time of the PCBs
	unsigned long total_run_time; // the total time to process all the PCBs in the ready queue
}	ScheduleResult_t;

// Create and Define worker input struct
// that is needed for thread worker function below
typedef struct {
	
} WorkerInput_t;


// Runs the First Come First Serve Process Scheduling over the incoming ready_queue
// \param ready queue a dyn_array of type ProcessControlBlock_t that contain be up to N elements 
// \param result used for first come first serve stat tracking \ref ScheduleResult_t
// \return true if function ran successful else false for an error
bool first_come_first_serve(dyn_array_t* ready_queue, ScheduleResult_t* result);

// Runs the Round Robin Process Scheduling over the incoming ready_queue
// \param ready queue a dyn_array of type ProcessControlBlock_t that contain be up to N elements 
// \param result used for first come first serve stat tracking \ref ScheduleResult_t
// \return true if function ran successful else false for an error
bool round_robin(dyn_array_t* ready_queue, ScheduleResult_t* result);

// Reads the PCB burst time values from the binary file into ProcessControlBlock_t remaining_burst_time field
// for N number of PCB burst time stored in the file.
// \param input_file the file containing the PCB burst times
// \return a populated dyn_array of ProcessControlBlocks if function ran successful else NULL for an error
dyn_array_t* load_process_control_blocks (const char* input_file );

// The function that will be threaded for running first_come_first_serve in parallel
// \param input is a user defined structure that contains a pointer reference to the 
//		shared dyn_array of ProcessControlBlock_t and a pointer to a non shared ScheduleResult_t struct
// \return nothing
void* first_come_first_serve_worker (void* input);

// The function that will be threaded for running round_robin in parallel
// \param input is a user defined structure that contains a pointer reference to the 
//		shared dyn_array of ProcessControlBlock_t and a pointer to a non shared ScheduleResult_t struct
// \return nothing
void* round_robin_worker (void* input);

// init the protected mutex
bool init_lock(void);
#endif
