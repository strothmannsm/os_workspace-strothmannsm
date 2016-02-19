#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <dyn_array.h>
#include "../include/processing_scheduling.h"


// private function
void virtual_cpu(ProcessControlBlock_t* process_control_block) {
	// decrement the burst time of the pcb
	--process_control_block->remaining_burst_time;
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

	return true;
}


