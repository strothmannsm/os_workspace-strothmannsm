#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <dyn_array.h>
#include "../include/processing_scheduling.h"

#define QUANTUM 4 // Used for Robin Round for process as the run time limit

//global lock variable
pthread_mutex_t mutex;

// private function
void virtual_cpu(ProcessControlBlock_t* process_control_block) {
	// decrement the burst time of the pcb
	--process_control_block->remaining_burst_time;
	sleep(1);
}

void destroy_mutex (void) {
	pthread_mutex_destroy(&mutex);	
};	

// init the protected mutex
bool init_lock(void) {
	if (pthread_mutex_init(&mutex,NULL) != 0) {
		return false;
	}
	atexit(destroy_mutex);
	return true;
}

bool first_come_first_serve(dyn_array_t* ready_queue, ScheduleResult_t* result) {

	if(!ready_queue || !result) {
		return false;
	}

	size_t n_proc = dyn_array_size(ready_queue); //find out how many processes are in queue
	uint32_t clocktime = 0; // initialize clocktime
	float avg_latency = 0;
	float avg_wallclock = 0;

	/* While there are process in the ready queue
	*  extract the PCB from the back and run it on the CPU
	*  until it's done.
	*/
	while(dyn_array_size(ready_queue) > 0) {
		ProcessControlBlock_t pcb;
		if(dyn_array_extract_back(ready_queue, &pcb)) { // get next process out of the queue
			avg_latency += clocktime; // process headed to cpu, add latency
			// run process until completed
			while(pcb.remaining_burst_time > 0) {
				virtual_cpu(&pcb);
				++clocktime; // another cycle has passed, increment
			}
			avg_wallclock += clocktime; // process finished, add wallclock
		} else {
			return false; // failed to extract pcb from queue
		}
	}

	// average out latency and wallclock
	avg_latency /= n_proc;
	avg_wallclock /= n_proc;

	// set timing results
	result->average_latency_time = avg_latency;
	result->average_wall_clock_time = avg_wallclock;
	result->total_run_time = clocktime;
}

bool round_robin(dyn_array_t* ready_queue, ScheduleResult_t* result) {

	if(ready_queue && result) {
		size_t n_proc = dyn_array_size(ready_queue); //find out how many processes are in queue
		uint32_t clocktime = 0; // initialize clocktime
		size_t i = 0; // helper for latency
		float avg_latency = 0.0;
		float avg_wallclock = 0.0;

		/* While there are processess in the ready_queue
		*  	-extract the process from the "head"
		*  	-get latency, if not already calculated
		*  	-Run the process on the virtual_cpu for QUANTUM time
		*  		or until it completes, whichever comes first
		*  	-if it didn't complete put it at the "back" of the queue
		*  		else get wallclock 
		*/
		while(dyn_array_size(ready_queue) > 0) {
			ProcessControlBlock_t pcb;
			if(dyn_array_extract_back(ready_queue, &pcb)) {
				// when i = n_proc, all latencies have been calculated
				if(i < n_proc) { 
					avg_latency += clocktime;
					i++;
				}
				
				int j = 0;
				while(j < QUANTUM && pcb.remaining_burst_time > 0) {
					virtual_cpu(&pcb);
					++clocktime;
					++j;
				}

				if(pcb.remaining_burst_time > 0) {
					dyn_array_push_front(ready_queue, &pcb);
				} else {
					avg_wallclock += clocktime;
				}
			} else {
				return false;
			}
		}

		// put final timing results in result
		result->average_latency_time = avg_latency/n_proc;
		result->average_wall_clock_time = avg_wallclock/n_proc;
		result->total_run_time = clocktime;

		return true;
	}

	return false;
}
/*
* MILESTONE 3 CODE
*/
dyn_array_t* load_process_control_blocks (const char* input_file ) {
	
}

void* first_come_first_serve_worker (void* input) {
	
}

void* round_robin_worker (void* input) {
	
>>>>>>> upstream/master
}

