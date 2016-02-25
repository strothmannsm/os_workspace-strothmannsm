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

		pthread_mutex_lock(&mutex);
		bool res = dyn_array_extract_back(ready_queue, &pcb);
		pthread_mutex_unlock(&mutex);

		if(res) {
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

			pthread_mutex_lock(&mutex);
			bool res = dyn_array_extract_back(ready_queue, &pcb);
			pthread_mutex_unlock(&mutex);

			if(res) {
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
	// check for invalid filename input
	if(!input_file || !strcmp(input_file,"") || !strcmp(input_file,"\n")) {
		return (dyn_array_t*)NULL;
	}
	// open the file in read only mode
	int fd = open(input_file, O_RDONLY);
	if(fd == -1) { // file didn't open
		return (dyn_array_t*)NULL;
	}
	
	//read the first number from the file, this is the number of processes
	uint32_t n_proc;
	int bytes_read = read(fd, &n_proc, sizeof(uint32_t));

	// validate that read happened
	if(bytes_read == -1 || bytes_read == 0) {
		close(fd);
		return (dyn_array_t*)NULL;
	}

	// create ready_queue dyn_array and array to read burst times from file
	dyn_array_t* ready_queue = dyn_array_create(0,sizeof(ProcessControlBlock_t),NULL);
	uint32_t burst_times[n_proc];

	//read burst times and validate that we read enough data
	bytes_read = read(fd, &burst_times, n_proc * sizeof(uint32_t));
	if(bytes_read == -1 || bytes_read != (int)(n_proc * sizeof(uint32_t))) {
		close(fd);
		dyn_array_destroy(ready_queue);
		return (dyn_array_t*)NULL;
	}
	close(fd);

	ProcessControlBlock_t data[n_proc];

	//place the burst times into ProcessControlBlock_t and push onto the ready_queue
	for(uint32_t i = 0; i < n_proc; i++) {
		data[i].remaining_burst_time = burst_times[i];
		data[i].started = 0;
		dyn_array_push_back(ready_queue, &data[i]);
	}
	
	return ready_queue;
}

void* first_come_first_serve_worker (void* input) {
	// cast input so we can use it
	WorkerInput_t* worker_input = (WorkerInput_t*)input;
	
	// unpack ready_queue and result struct
	dyn_array_t* ready_queue = worker_input->ready_queue;
	ScheduleResult_t* result = worker_input->result;
	
	// send to FCFS
	first_come_first_serve(ready_queue, result);

	pthread_exit((void*)0);
}

void* round_robin_worker (void* input) {
	// cast input so we can use it
	WorkerInput_t* worker_input = (WorkerInput_t*)input;

	// unpack ready_queue and result struct
	dyn_array_t* ready_queue = worker_input->ready_queue;
	ScheduleResult_t* result = worker_input->result;
	
	// send to RR
	round_robin(ready_queue, result);

	pthread_exit((void*)0);
}

